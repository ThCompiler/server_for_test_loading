FROM gcc:latest as build

RUN apt-get update && apt-get install -y cmake

WORKDIR /app

COPY . .

WORKDIR /app/build

RUN cmake .. && \
    cmake --build . --config Release


FROM gcc:latest

WORKDIR /app

ARG PORT

ENV USE_PORT=$PORT

EXPOSE $PORT

COPY /httptest /httptest

COPY --from=build /app/build/httpd .

CMD ./httpd -p $USE_PORT