ARG UBUNTU_IMAGE=ubuntu:24.04
ARG NODE_IMAGE=node:22-bookworm-slim
ARG NGINX_IMAGE=nginx:stable-alpine
FROM ${UBUNTU_IMAGE} AS backend-build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates git cmake g++ make libjsoncpp-dev uuid-dev zlib1g-dev libssl-dev libmariadb-dev libbrotli-dev libc-ares-dev && rm -rf /var/lib/apt/lists/*
ARG DROGON_VERSION=v1.9.11
RUN git clone --branch ${DROGON_VERSION} --depth 1 --recurse-submodules --shallow-submodules https://github.com/drogonframework/drogon.git /tmp/drogon && cmake -S /tmp/drogon -B /tmp/drogon/build -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF -DBUILD_CTL=ON && cmake --build /tmp/drogon/build -j2 && cmake --install /tmp/drogon/build
WORKDIR /src
COPY CMakeLists.txt main.cc ./
COPY controllers controllers
COPY filters filters
COPY plugins plugins
COPY models models
COPY services services
COPY utils utils
COPY views views
COPY test test
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --target erp_system -j2

FROM ${UBUNTU_IMAGE} AS backend
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates libjsoncpp25 libuuid1 zlib1g libssl3t64 libmariadb3 libbrotli1 libc-ares2 python3 mariadb-client && rm -rf /var/lib/apt/lists/* && useradd --uid 10001 --create-home erp
COPY --from=backend-build /usr/local/lib/ /usr/local/lib/
RUN ldconfig
WORKDIR /app
COPY --from=backend-build /src/build/erp_system /app/erp_system
COPY deploy/entrypoint.py /app/entrypoint.py
COPY deploy/config.template.json /app/config.template.json
COPY db /app/db
RUN mkdir /app/uploads && chown -R erp:erp /app
USER erp
EXPOSE 5555
ENTRYPOINT ["python3", "/app/entrypoint.py"]
CMD ["./erp_system"]

FROM ${NODE_IMAGE} AS frontend-build
WORKDIR /src
COPY frontend/package*.json ./
RUN npm ci
COPY frontend/ ./
RUN npm run build

FROM ${NGINX_IMAGE} AS frontend
COPY --from=frontend-build /src/dist /usr/share/nginx/html
COPY deploy/nginx.conf /etc/nginx/conf.d/default.conf
EXPOSE 80
