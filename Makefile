PROJECT = httpd
CONTAINER = server
PORT = 8081
NGINX_PORT = 8080

CORE_NUMBER = 2

build:
	mkdir build
	cd build && cmake ..

run-nginx-benchmark:
	wrk -t12 -c400 -d30s 'http://127.0.0.1:$(NGINX_PORT)/httptest/splash.css'

run-httpd-benchmark:
	wrk -t12 -c400 -d30s 'http://127.0.0.1:$(PORT)/httptest/splash.css'

run-func-test:
	python3 ./httptest.py

build-docker-nginx:
	docker build -t nginx-local -f nginx.Dockerfile .

docker-run-nginx:
	docker run -p $(NGINX_PORT):$(NGINX_PORT) --name nginx-local -t nginx-local

docker-stop-nginx:
	docker stop nginx-local

build-docker:
	docker build --no-cache . --tag $(PROJECT) --build-arg PORT=$(PORT)

docker-run:
	docker run -it --cpus="$(CORE_NUMBER)" -p $(PORT):$(PORT) --name $(CONTAINER) -t $(PROJECT)

docker-stop:
	docker stop $(CONTAINER)

docker-free:
	docker rm -fv $$(docker ps -aq) || true