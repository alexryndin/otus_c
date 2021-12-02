# hw8
## Утилита clib-build
### Начальный анализ утечек:
```
$ valgrind --leak-check=full --show-leak-kinds=definite ./clib-build
<...>
==294200==
==294200== HEAP SUMMARY:
==294200==     in use at exit: 1,427,856 bytes in 21,199 blocks
==294200==   total heap usage: 84,057 allocs, 62,858 frees, 19,397,400 bytes allocated
==294200==
==294200== 9 bytes in 1 blocks are definitely lost in loss record 11 of 258
==294200==    at 0x483E7C5: malloc (vg_replace_malloc.c:380)
==294200==    by 0x142807: substr (substr.c:30)
==294200==    by 0x13CB7A: parse_repo_owner (parse-repo.c:32)
==294200==    by 0x11D54C: clib_package_new_from_slug_with_package_name (clib-package.c:614)
==294200==    by 0x11DE9D: clib_package_new_from_slug (clib-package.c:796)
==294200==    by 0x121779: build_package_with_manifest_name (clib-build.c:339)
==294200==    by 0x121DCF: build_package (clib-build.c:485)
==294200==    by 0x120B57: build_package_with_manifest_name_thread (clib-build.c:127)
==294200==    by 0x4924258: start_thread (in /usr/lib/libpthread-2.33.so)
==294200==    by 0x4A3A5E2: clone (in /usr/lib/libc-2.33.so)
==294200==
==294200== 1,300 bytes in 1 blocks are definitely lost in loss record 173 of 258
==294200==    at 0x483E7C5: malloc (vg_replace_malloc.c:380)
==294200==    by 0x12529F: fs_fnread (fs.c:198)
==294200==    by 0x12527E: fs_fread (fs.c:192)
==294200==    by 0x1251DA: fs_read (fs.c:173)
==294200==    by 0x120C64: build_package_with_manifest_name (clib-build.c:163)
==294200==    by 0x121DCF: build_package (clib-build.c:485)
==294200==    by 0x1229E8: main (clib-build.c:698)
==294200==
==294200== 1,410 (960 direct, 450 indirect) bytes in 30 blocks are definitely lost in loss record 177 of 258
==294200==    at 0x483E7C5: malloc (vg_replace_malloc.c:380)
==294200==    by 0x13BE79: http_get_shared (http-get.c:46)
==294200==    by 0x11D7DD: clib_package_new_from_slug_with_package_name (clib-package.c:660)
==294200==    by 0x11DE9D: clib_package_new_from_slug (clib-package.c:796)
==294200==    by 0x121779: build_package_with_manifest_name (clib-build.c:339)
==294200==    by 0x121DCF: build_package (clib-build.c:485)
==294200==    by 0x1229E8: main (clib-build.c:698)
==294200==
==294200== 1,974 (1,344 direct, 630 indirect) bytes in 42 blocks are definitely lost in loss record 185 of 258
==294200==    at 0x483E7C5: malloc (vg_replace_malloc.c:380)
==294200==    by 0x13BE79: http_get_shared (http-get.c:46)
==294200==    by 0x11D7DD: clib_package_new_from_slug_with_package_name (clib-package.c:660)
==294200==    by 0x11DE9D: clib_package_new_from_slug (clib-package.c:796)
==294200==    by 0x121779: build_package_with_manifest_name (clib-build.c:339)
==294200==    by 0x121DCF: build_package (clib-build.c:485)
==294200==    by 0x120B57: build_package_with_manifest_name_thread (clib-build.c:127)
==294200==    by 0x4924258: start_thread (in /usr/lib/libpthread-2.33.so)
==294200==    by 0x4A3A5E2: clone (in /usr/lib/libc-2.33.so)
==294200==
==294200== LEAK SUMMARY:
==294200==    definitely lost: 3,613 bytes in 74 blocks
==294200==    indirectly lost: 1,080 bytes in 72 blocks
==294200==      possibly lost: 1,401,632 bytes in 20,867 blocks
==294200==    still reachable: 21,531 bytes in 186 blocks
==294200==         suppressed: 0 bytes in 0 blocks
==294200== Reachable blocks (those to which a pointer was found) are not shown.
==294200== To see them, rerun with: --leak-check=full --show-leak-kinds=all
==294200==
==294200== For lists of detected and suppressed errors, rerun with: -s
==294200== ERROR SUMMARY: 214 errors from 214 contexts (suppressed: 0 from 0)
```
### Патч 1
Направлен на исправление утечки, вызванной невысвобождением памяти, полученной
при чтении json с диска через библиотеку fs.c:
```
diff --git a/hw8/clib/src/clib-build.c b/hw8/clib/src/clib-build.c
index 4372bdb..e492d2f 100644
--- a/hw8/clib/src/clib-build.c
+++ b/hw8/clib/src/clib-build.c
@@ -167,6 +167,9 @@ int build_package_with_manifest_name(const char *dir, const char *file) {
       root_package = clib_package_new(json, opts.verbose);
     }

+    free(json);
+    json = NULL;
+
     if (root_package && root_package->prefix) {
       char prefix[path_max];
       memset(prefix, 0, path_max);
@@ -201,6 +204,8 @@ int build_package_with_manifest_name(const char *dir, const char *file) {
 #else
     package = clib_package_new(json, 0);
 #endif
+    free(json);
+    json = NULL;
   } else {
 #ifdef DEBUG
     package = clib_package_new_from_slug(dir, 1);

```
Сообщение, которое подсказало где искать ошибку:
```
1,300 bytes in 1 blocks are definitely lost in loss record 173 of 258
   at 0x483E7C5: malloc (vg_replace_malloc.c:380)
   by 0x12529F: fs_fnread (fs.c:198)
   by 0x12527E: fs_fread (fs.c:192)
   by 0x1251DA: fs_read (fs.c:173)
   by 0x120C64: build_package_with_manifest_name (clib-build.c:163)
   by 0x121DCF: build_package (clib-build.c:485)
   by 0x1229E8: main (clib-build.c:698)
```
### Патч 2
Направлен на исправление утечки, вызванной невысвобождением памяти,
содержащей имя автора пакета:
```
diff --git a/hw8/clib/src/common/clib-package.c b/hw8/clib/src/common/clib-package.c
index 268538e..60af6e5 100644
--- a/hw8/clib/src/common/clib-package.c
+++ b/hw8/clib/src/common/clib-package.c
@@ -712,6 +712,8 @@ clib_package_new_from_slug_with_package_name(const char *slug, int verbose,
     }
   } else {
     pkg->author = strdup(author);
+    free(author);
+    author = NULL;
   }

   if (!(repo = clib_package_repo(pkg->author, pkg->name))) {

```
Сообщение, которое подсказало где искать ошибку:
```
9 bytes in 1 blocks are definitely lost in loss record 11 of 258
   at 0x483E7C5: malloc (vg_replace_malloc.c:380)
   by 0x142807: substr (substr.c:30)
   by 0x13CB7A: parse_repo_owner (parse-repo.c:32)
   by 0x11D54C: clib_package_new_from_slug_with_package_name (clib-package.c:614)
   by 0x11DE9D: clib_package_new_from_slug (clib-package.c:796)
   by 0x121779: build_package_with_manifest_name (clib-build.c:339)
   by 0x121DCF: build_package (clib-build.c:485)
   by 0x120B57: build_package_with_manifest_name_thread (clib-build.c:127)
   by 0x4924258: start_thread (in /usr/lib/libpthread-2.33.so)
   by 0x4A3A5E2: clone (in /usr/lib/libc-2.33.so)
```
### Патч 3
Направлен на исправление утечки, вызванной невысвобождением памяти
после http-запроса, вернувшего не ок:
```
diff --git a/hw8/clib/src/common/clib-package.c b/hw8/clib/src/common/clib-package.c
index 60af6e5..19bcc1d 100644
--- a/hw8/clib/src/common/clib-package.c
+++ b/hw8/clib/src/common/clib-package.c
@@ -661,11 +661,15 @@ clib_package_new_from_slug_with_package_name(const char *slug, int verbose,
 #else
       res = http_get(json_url);
 #endif
-      json = res->data;
       _debug("status: %d", res->status);
       if (!res || !res->ok) {
+        if (res) {
+          http_get_free(res);
+          res = NULL;
+        }
         goto download;
       }
+      json = res->data;
       log = "fetch";
     }
   }
```
Сообщение, которое подсказало где искать ошибку:
```
9 bytes in 1 blocks are definitely lost in loss record 11 of 258
   at 0x483E7C5: malloc (vg_replace_malloc.c:380)
   by 0x142807: substr (substr.c:30)
   by 0x13CB7A: parse_repo_owner (parse-repo.c:32)
   by 0x11D54C: clib_package_new_from_slug_with_package_name (clib-package.c:614)
   by 0x11DE9D: clib_package_new_from_slug (clib-package.c:796)
   by 0x121779: build_package_with_manifest_name (clib-build.c:339)
   by 0x121DCF: build_package (clib-build.c:485)
   by 0x120B57: build_package_with_manifest_name_thread (clib-build.c:127)
   by 0x4924258: start_thread (in /usr/lib/libpthread-2.33.so)
   by 0x4A3A5E2: clone (in /usr/lib/libc-2.33.so)
```
#### Запуск после патчинга:
```
==13359== HEAP SUMMARY:
==13359==     in use at exit: 1,423,478 bytes in 21,061 blocks
==13359==   total heap usage: 84,082 allocs, 63,021 frees, 19,382,585 bytes allocated
==13359==
==13359== LEAK SUMMARY:
==13359==    definitely lost: 0 bytes in 0 blocks
==13359==    indirectly lost: 0 bytes in 0 blocks
==13359==      possibly lost: 1,401,947 bytes in 20,875 blocks
==13359==    still reachable: 21,531 bytes in 186 blocks
==13359==         suppressed: 0 bytes in 0 blocks
==13359== Reachable blocks (those to which a pointer was found) are not shown.
==13359== To see them, rerun with: --leak-check=full --show-leak-kinds=all
==13359==
==13359== For lists of detected and suppressed errors, rerun with: -s
==13359== ERROR SUMMARY: 210 errors from 210 contexts (suppressed: 0 from 0)
```
## Вопросы:
Очень много ошибок, совершенно непонятных и непонятно чем вызванных, подобных
вот таким:
```
==17044== 83,632 bytes in 428 blocks are possibly lost in loss record 249 of 253
==17044==    at 0x483E7C5: malloc (vg_replace_malloc.c:380)
==17044==    by 0x4DBF14A: CRYPTO_zalloc (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CF7CCE: ??? (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CF80B5: BN_bin2bn (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CDC0E9: ??? (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CD6CF8: ??? (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CD7E3C: ??? (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CD8375: ??? (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CD7C1F: ??? (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CD873D: ASN1_item_ex_d2i (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CD87BB: ASN1_item_d2i (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4DED1DF: ??? (in /usr/lib/libcrypto.so.1.1)
==17044==
==17044== 85,988 bytes in 262 blocks are possibly lost in loss record 250 of 253
==17044==    at 0x483E7C5: malloc (vg_replace_malloc.c:380)
==17044==    by 0x4CC545C: ??? (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CD7284: ??? (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CD7E3C: ??? (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CD8375: ??? (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CD7C1F: ??? (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CD873D: ASN1_item_ex_d2i (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4CD87BB: ASN1_item_d2i (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4DD0C27: PEM_X509_INFO_read_bio (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4E33874: X509_load_cert_crl_file (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4E339C2: ??? (in /usr/lib/libcrypto.so.1.1)
==17044==    by 0x4E36901: X509_STORE_load_locations (in /usr/lib/libcrypto.so.1.1)
```
Часто фигурируют ссылки на различные функции, связанные с шифрованием,
и различные `???`, отчего делаю предположение, что "вероятная" потеря
данных связана с особенностями работы криптографических библиотек и их
заботой об обфускации и заметании следов.

## Ссылки на патчи:

