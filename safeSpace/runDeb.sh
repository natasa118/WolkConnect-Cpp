 #!/usr/bin/env bash

docker container stop runnabletask
docker container rm runnabletask

docker build . -t rundeb -f ./Dockerfile_PlusRunDeb

docker run -dit --name runnabletask --cpus $(nproc) rundeb || exit

docker exec -it runnabletask /opt/wolkabout/assignment/out/bin/ip_tracker /opt/wolkabout/assignment/configuration/conf.json

docker container stop runnabletask
docker container rm runnabletask
