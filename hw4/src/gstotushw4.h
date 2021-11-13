/* GStreamer
 * Copyright (C) 2021 FIXME <fixme@example.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_OTUSHW4_H_
#define _GST_OTUSHW4_H_

#include <gst/audio/gstaudiosrc.h>
#include <stdio.h>

G_BEGIN_DECLS

#define GST_TYPE_OTUSHW4 (gst_otushw4_get_type())
#define GST_OTUSHW4(obj)                                                       \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_OTUSHW4, GstOtushw4))
#define GST_OTUSHW4_CLASS(klass)                                               \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_OTUSHW4, GstOtushw4Class))
#define GST_IS_OTUSHW4(obj)                                                    \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_OTUSHW4))
#define GST_IS_OTUSHW4_CLASS(obj)                                              \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_OTUSHW4))

typedef struct _GstOtushw4 GstOtushw4;
typedef struct _GstOtushw4Class GstOtushw4Class;

#pragma pack(1)
struct _GstOtushw4 {
    GstAudioSrc base_otushw4;
    gchar *filepath;
    int input;
#pragma pack(1)
    struct WavHeader {
        // WAV-формат начинается с RIFF-заголовка:

        // Содержит символы "RIFF" в ASCII кодировке
        // (0x52494646 в big-endian представлении)
        char chunkId[4];

        // 36 + subchunk2Size, или более точно:
        // 4 + (8 + subchunk1Size) + (8 + subchunk2Size)
        // Это оставшийся размер цепочки, начиная с этой позиции.
        // Иначе говоря, это размер файла - 8, то есть,
        // исключены поля chunkId и chunkSize.
        unsigned long chunkSize;

        // Содержит символы "WAVE"
        // (0x57415645 в big-endian представлении)
        char format[4];

        // Формат "WAVE" состоит из двух подцепочек: "fmt " и "data":
        // Подцепочка "fmt " описывает формат звуковых данных:

        // Содержит символы "fmt "
        // (0x666d7420 в big-endian представлении)
        char subchunk1Id[4];

        // 16 для формата PCM.
        // Это оставшийся размер подцепочки, начиная с этой позиции.
        unsigned long subchunk1Size;

        //  PCM = 1 (то есть, Линейное квантование). Зн
        // чения, отличающиеся от 1, обозначают некоторый формат
        // сжатия.
        unsigned short audioFormat;

        // Количество каналов. Моно = 1, Стерео = 2 и т.д.
        unsigned short numChannels;

        // Частота дискретизации. 8000 Гц, 44100 Гц и т.д.
        unsigned long sampleRate;

        // sampleRate * numChannels * bitsPerSample/8
        unsigned long byteRate;

        // numChannels * bitsPerSample/8
        // Количество байт для одного сэмпла, включая все каналы.
        unsigned short blockAlign;

        // Так называемая "глубиная" или точность звучания. 8 бит, 16 бит и т.д.
        unsigned short bitsPerSample;

        // Подцепочка "data" содержит аудио-данные и их размер.

        // Содержит символы "data"
        // (0x64617461 в big-endian представлении)
        char subchunk2Id[4];

        // numSamples * numChannels * bitsPerSample/8
        // Количество байт в области данных.
        unsigned long subchunk2Size;
    } WavHeader;

    // Далее следуют непосредственно Wav данные.
};

struct _GstOtushw4Class {
    GstAudioSrcClass base_otushw4_class;
};

GType gst_otushw4_get_type(void);

G_END_DECLS

#endif
