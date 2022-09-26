FROM alpine:latest
RUN set -x \
    && apk update \
    && apk upgrade --no-cache \
    && apk add g++ sqlite-dev
ENTRYPOINT [ "/entrypoint.sh" ]
