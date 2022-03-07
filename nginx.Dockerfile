FROM nginx

WORKDIR /etc/nginx

COPY nginx.conf /etc/nginx/nginx.conf
COPY  ./httptest /etc/nginx/static