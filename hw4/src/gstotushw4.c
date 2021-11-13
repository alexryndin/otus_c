/* GStreamer
 * Copyright (C) 2021 Alex Ryndin <me@alexryndin.me>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstotushw4
 *
 * The otushw4 element does wav decoding stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 otushw4 filepath=test.wav ! autoaudiosink
 * ]|
 * This pipeline reads test wav file and send its data to autoaudiosink
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>

#include "gstotushw4.h"
#include <fcntl.h>
#include <glib/gstdio.h>
#include <gst/audio/gstaudiosrc.h>
#include <gst/gst.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

GST_DEBUG_CATEGORY_STATIC(gst_otushw4_debug_category);
#define GST_CAT_DEFAULT gst_otushw4_debug_category

#ifndef O_BINARY
#define O_BINARY (0)
#endif

#define SEC_SIZE 96000
#define SAMPLE_SIZE 2
#define HEADER_SIZE 44

const unsigned char CHUNKID_SIG[] = {'R', 'I', 'F', 'F'};
const unsigned char SUBCHUNK1ID_SIG[] = {'f', 'm', 't', ' '};
const unsigned char SUBCHUNK2ID_SIG[] = {'d', 'a', 't', 'a'};
const unsigned char FORMAT_SIG[] = {'W', 'A', 'V', 'E'};

/* prototypes */

static void gst_otushw4_set_property(GObject *object, guint property_id,
                                     const GValue *value, GParamSpec *pspec);
static void gst_otushw4_get_property(GObject *object, guint property_id,
                                     GValue *value, GParamSpec *pspec);
static void gst_otushw4_dispose(GObject *object);
static void gst_otushw4_finalize(GObject *object);

static gboolean gst_otushw4_open(GstAudioSrc *src);
static gboolean gst_otushw4_prepare(GstAudioSrc *src,
                                    GstAudioRingBufferSpec *spec);
static gboolean gst_otushw4_unprepare(GstAudioSrc *src);
static gboolean gst_otushw4_close(GstAudioSrc *src);
static guint gst_otushw4_read(GstAudioSrc *src, gpointer data, guint length,
                              GstClockTime *timestamp);
static guint gst_otushw4_delay(GstAudioSrc *src);
static void gst_otushw4_reset(GstAudioSrc *src);

enum { PROP_0, PROP_FILEPATH };

/* pad templates */

/* FIXME add/remove the formats that you want to support */
static GstStaticPadTemplate gst_otushw4_src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS("audio/x-raw,format=S16LE,rate=48000,"
                    "channels=1,layout=interleaved"));

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(
    GstOtushw4, gst_otushw4, GST_TYPE_AUDIO_SRC,
    GST_DEBUG_CATEGORY_INIT(gst_otushw4_debug_category, "otushw4", 0,
                            "debug category for otushw4 element"));

static void gst_otushw4_class_init(GstOtushw4Class *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstAudioSrcClass *audio_src_class = GST_AUDIO_SRC_CLASS(klass);

    /* Setting up pads and setting metadata should be moved to
       base_class_init if you intend to subclass this class. */
    gst_element_class_add_static_pad_template(GST_ELEMENT_CLASS(klass),
                                              &gst_otushw4_src_template);

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass), "OtusHW4",
                                          "Generic", "OtusHW4",
                                          "Alex Ryndin <me@alexryndin.me>");

    gobject_class->set_property = gst_otushw4_set_property;
    gobject_class->get_property = gst_otushw4_get_property;
    gobject_class->dispose = gst_otushw4_dispose;
    gobject_class->finalize = gst_otushw4_finalize;
    audio_src_class->open = GST_DEBUG_FUNCPTR(gst_otushw4_open);
    audio_src_class->prepare = GST_DEBUG_FUNCPTR(gst_otushw4_prepare);
    audio_src_class->unprepare = GST_DEBUG_FUNCPTR(gst_otushw4_unprepare);
    audio_src_class->close = GST_DEBUG_FUNCPTR(gst_otushw4_close);
    audio_src_class->read = GST_DEBUG_FUNCPTR(gst_otushw4_read);
    audio_src_class->delay = GST_DEBUG_FUNCPTR(gst_otushw4_delay);
    audio_src_class->reset = GST_DEBUG_FUNCPTR(gst_otushw4_reset);

    g_object_class_install_property(
        gobject_class, PROP_FILEPATH,
        g_param_spec_string("filepath", "filepath", "filepath of the audio", "",
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void gst_otushw4_init(GstOtushw4 *otushw4) {}

void gst_otushw4_set_property(GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec) {
    GstOtushw4 *otushw4 = GST_OTUSHW4(object);

    GST_DEBUG_OBJECT(otushw4, "set_property");

    switch (property_id) {
    case PROP_FILEPATH:
        otushw4->filepath = g_strdup(g_value_get_string(value));
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_otushw4_get_property(GObject *object, guint property_id, GValue *value,
                              GParamSpec *pspec) {
    GstOtushw4 *otushw4 = GST_OTUSHW4(object);

    GST_DEBUG_OBJECT(otushw4, "get_property");

    switch (property_id) {
    case PROP_FILEPATH:
        g_value_set_string(value, otushw4->filepath);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_otushw4_dispose(GObject *object) {
    GstOtushw4 *otushw4 = GST_OTUSHW4(object);

    GST_DEBUG_OBJECT(otushw4, "dispose");

    /* clean up as possible.  may be called multiple times */

    G_OBJECT_CLASS(gst_otushw4_parent_class)->dispose(object);
}

void gst_otushw4_finalize(GObject *object) {
    GstOtushw4 *otushw4 = GST_OTUSHW4(object);

    GST_DEBUG_OBJECT(otushw4, "finalize");

    /* clean up object here */
    GST_DEBUG("clean up filepath");
    if (otushw4->filepath != NULL) {
        free(otushw4->filepath);
    }

    G_OBJECT_CLASS(gst_otushw4_parent_class)->finalize(object);
}

/* open the device with given specs */
static gboolean gst_otushw4_open(GstAudioSrc *src) {
    GstOtushw4 *otushw4 = GST_OTUSHW4(src);

    GST_DEBUG_OBJECT(otushw4, "open");

    int flags = O_RDONLY | O_BINARY;

    otushw4->input = g_open(otushw4->filepath, flags, 0);
    if (otushw4->input < 0) {
        goto error_exit;
    }

    int ret = 0;

   // ret += read(otushw4->input, &otushw4->WavHeader.chunkId, 4);
   // ret += read(otushw4->input, &otushw4->WavHeader.chunkSize, 4);
   // ret += read(otushw4->input, &otushw4->WavHeader.format, 4);
   // ret += read(otushw4->input, &otushw4->WavHeader.subchunk1Id, 4);
   // ret += read(otushw4->input, &otushw4->WavHeader.subchunk1Size, 4);
   // ret += read(otushw4->input, &otushw4->WavHeader.audioFormat, 2);
   // ret += read(otushw4->input, &otushw4->WavHeader.numChannels, 2);
   // ret += read(otushw4->input, &otushw4->WavHeader.sampleRate, 4);
   // ret += read(otushw4->input, &otushw4->WavHeader.byteRate, 4);
   // ret += read(otushw4->input, &otushw4->WavHeader.blockAlign, 2);
   // ret += read(otushw4->input, &otushw4->WavHeader.bitsPerSample, 2);
   // ret += read(otushw4->input, &otushw4->WavHeader.subchunk2Id, 4);
   // ret += read(otushw4->input, &otushw4->WavHeader.subchunk2Size, 4);
   //
    ret += read(otushw4->input, &otushw4->WavHeader, HEADER_SIZE);

    GST_INFO("audio format %d", otushw4->WavHeader.audioFormat);
    GST_INFO("CHUNK ID %.*s", 4, otushw4->WavHeader.chunkId);
    GST_INFO("FORMAT %.*s", 4, otushw4->WavHeader.format);
    GST_INFO("SUBCHUNK1ID_SIG %.*s", 4, otushw4->WavHeader.subchunk1Id);
    GST_INFO("SUBCHUNK2ID_SIG %.*s", 4, otushw4->WavHeader.subchunk2Id);
    GST_INFO("Sample rate %lu byteRate %lu, BlockAlign %d bitspersample %d",
             otushw4->WavHeader.sampleRate, otushw4->WavHeader.byteRate,
             otushw4->WavHeader.blockAlign, otushw4->WavHeader.bitsPerSample);
    if (ret < HEADER_SIZE ||
        memcmp(otushw4->WavHeader.chunkId, CHUNKID_SIG, 4) ||
        memcmp(otushw4->WavHeader.format, FORMAT_SIG, 4) ||
        memcmp(otushw4->WavHeader.subchunk1Id, SUBCHUNK1ID_SIG, 4) ||
        otushw4->WavHeader.subchunk1Size != 16 ||
        otushw4->WavHeader.audioFormat != 1 ||
        otushw4->WavHeader.numChannels != 1 ||
        memcmp(otushw4->WavHeader.subchunk2Id, SUBCHUNK2ID_SIG, 4)) {
        GST_ERROR("Wrong wav format");
        goto error_exit;
    }
    GST_INFO("CHUNK ID %.*s", 4, otushw4->WavHeader.chunkId);

    return TRUE;
error_exit:
    GST_ELEMENT_ERROR(src, RESOURCE, READ, (NULL), GST_ERROR_SYSTEM);
    return FALSE;
}

/* prepare resources and state to operate with the given specs */
static gboolean gst_otushw4_prepare(GstAudioSrc *src,
                                    GstAudioRingBufferSpec *spec) {
    GstOtushw4 *otushw4 = GST_OTUSHW4(src);

    GST_DEBUG_OBJECT(otushw4, "prepare");

    return TRUE;
}

/* undo anything that was done in prepare() */
static gboolean gst_otushw4_unprepare(GstAudioSrc *src) {
    GstOtushw4 *otushw4 = GST_OTUSHW4(src);

    GST_DEBUG_OBJECT(otushw4, "unprepare");

    return TRUE;
}

/* close the device */
static gboolean gst_otushw4_close(GstAudioSrc *src) {
    GstOtushw4 *otushw4 = GST_OTUSHW4(src);

    g_close(otushw4->input, NULL);

    GST_DEBUG_OBJECT(otushw4, "close");

    return TRUE;
}

/* read samples from the device */
static guint gst_otushw4_read(GstAudioSrc *src, gpointer data, guint length,
                              GstClockTime *timestamp) {
    GstOtushw4 *otushw4 = GST_OTUSHW4(src);

    int ret = 0;
    int delta = 0;
    GST_DEBUG("I was asked to push %d bytes of data, so i pushed %d, timestamp "
              "is %lu!",
              length, ret, *timestamp);
    while (ret < length) {
        delta += read(otushw4->input, data, length - ret);
        if (delta == 0) {
            break;
        }
        ret += delta;
    }
    // microsec
    int to_sleep = (length * 1000000) / SEC_SIZE;
    usleep(to_sleep);
    // nanosec
    // *timestamp += (to_sleep * 1000);
    GST_DEBUG("I was asked to push %d bytes of data, so i pushed %d, timestamp "
              "is %lu!",
              length, ret, *timestamp);

    GST_DEBUG_OBJECT(otushw4, "read");

    return ret;
}

/* get number of samples queued in the device */
static guint gst_otushw4_delay(GstAudioSrc *src) {
    GstOtushw4 *otushw4 = GST_OTUSHW4(src);

    GST_DEBUG_OBJECT(otushw4, "delay");

    return 0;
}

/* reset the audio device, unblock from a write */
static void gst_otushw4_reset(GstAudioSrc *src) {
    GstOtushw4 *otushw4 = GST_OTUSHW4(src);

    GST_DEBUG_OBJECT(otushw4, "reset");
    GST_INFO("reset");
}

static gboolean plugin_init(GstPlugin *plugin) {

    /* FIXME Remember to set the rank if it's an element that is meant
       to be autoplugged by decodebin. */
    return gst_element_register(plugin, "otushw4", GST_RANK_NONE,
                                GST_TYPE_OTUSHW4);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.FIXME"
#endif
#ifndef PACKAGE
#define PACKAGE "otushw"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "otushw"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://alexryndin.me/"
#endif

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, otushw4,
                  "FIXME plugin description", plugin_init, VERSION, "LGPL",
                  PACKAGE_NAME, GST_PACKAGE_ORIGIN)
