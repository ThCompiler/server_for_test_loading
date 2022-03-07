# Тестовый сервер отдачи статики

## Запуск тестов

### Запуск

#### Сервер

Для запуска сервера требуется выполнить следующие команды в корне проекта
```bash
make build-docker
make docker-run
```

Для остановки требуется следующие команды
```bash
make docker-stop
make docker-free
```

Для запуска с портом отличным от `8081` требуется указать параметр `PORT` при выполнении команд запуска
```bash
make build-docker PORT=8082
make docker-run PORT=8082
```


#### nginx

Для запуска nginx требуется выполнить следующие команды в корне проекта
```bash
make build-docker-nginx
make docker-run-nginx
```

Для остановки требуется следующие команды
```bash
make docker-stop-nginx
make docker-free
```

#### Функциональное тестирование

Для запуска функционального теста
```bash
make run-func-test
```

#### Нагрузочное тестирование

Для запуска нагрузочного теста nginx
```bash
make run-nginx-benchmark
```

Для запуска нагрузочного теста сервера
```bash
make run-httpd-benchmark
```

## Результаты тестов

### Функциональное тестирование

```text
python3 ./httptest.py  
```
Результат:
```bash
test_directory_index (__main__.HttpServer)
directory index file exists ... ok
test_document_root_escaping (__main__.HttpServer)
document root escaping forbidden ... ok
test_empty_request (__main__.HttpServer)
Send empty line ... ok
test_file_in_nested_folders (__main__.HttpServer)
file located in nested folders ... ok
test_file_not_found (__main__.HttpServer)
absent file returns 404 ... ok
test_file_type_css (__main__.HttpServer)
Content-Type for .css ... ok
test_file_type_gif (__main__.HttpServer)
Content-Type for .gif ... ok
test_file_type_html (__main__.HttpServer)
Content-Type for .html ... ok
test_file_type_jpeg (__main__.HttpServer)
Content-Type for .jpeg ... ok
test_file_type_jpg (__main__.HttpServer)
Content-Type for .jpg ... ok
test_file_type_js (__main__.HttpServer)
Content-Type for .js ... ok
test_file_type_png (__main__.HttpServer)
Content-Type for .png ... ok
test_file_type_swf (__main__.HttpServer)
Content-Type for .swf ... ok
test_file_urlencoded (__main__.HttpServer)
urlencoded filename ... ok
test_file_with_dot_in_name (__main__.HttpServer)
file with two dots in name ... ok
test_file_with_query_string (__main__.HttpServer)
query string with get params ... ok
test_file_with_slash_after_filename (__main__.HttpServer)
slash after filename ... ok
test_file_with_spaces (__main__.HttpServer)
filename with spaces ... ok
test_head_method (__main__.HttpServer)
head method support ... ok
test_index_not_found (__main__.HttpServer)
directory index file absent ... ok
test_large_file (__main__.HttpServer)
large file downloaded correctly ... ok
test_post_method (__main__.HttpServer)
post method forbidden ... ok
test_request_without_two_newlines (__main__.HttpServer)
Send GET without to newlines ... ok
test_server_header (__main__.HttpServer)
Server header exists ... ok

----------------------------------------------------------------------
Ran 24 tests in 6.769s

OK
```

### Нагрузочное тестирование тестирование

Нагрузочное тестирования **nginx**
```text
wrk -t12 -c400 -d30s 'http://127.0.0.1:8080/httptest/splash.css'   
```
Результат:
```bash
Running 30s test @ http://127.0.0.1:8080/httptest/splash.css
  12 threads and 400 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   176.15ms  208.81ms 894.45ms   79.78%
    Req/Sec   788.80    772.75     5.23k    74.77%
  139813 requests in 30.09s, 41.07MB read
  Non-2xx or 3xx responses: 139813
Requests/sec:   4647.03
Transfer/sec:      1.36MB
```

Нагрузочное тестирования сервера
```text
wrk -t12 -c400 -d30s 'http://127.0.0.1:8081/httptest/splash.css'   
```
Результат:
```bash
Running 30s test @ http://127.0.0.1:8081/httptest/splash.css
  12 threads and 400 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   274.03ms   28.16ms 477.30ms   94.50%
    Req/Sec   120.25     68.92   323.00     61.78%
  43246 requests in 30.09s, 3.79MB read
  Non-2xx or 3xx responses: 43246
Requests/sec:   1437.41
Transfer/sec:    129.14KB

```
