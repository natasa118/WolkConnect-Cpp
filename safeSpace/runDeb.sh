 #!/usr/bin/env bash

docker container stop runnabletask
docker container rm runnabletask

docker build . -t rundeb -f ./Dockerfile_PlusRunDeb

docker run -dit --privileged --name runnabletask --cpus $(nproc) rundeb || exit
docker cp runnabletask:/build/ .

docker container stop runnabletask
docker container rm runnabletask
