#!/bin/bash

rm -rf /build
rm *.deb
./make_zip.sh

docker container stop debuilder
docker container rm debuilder

docker build -t mytask .

docker run -dit --name debuilder --cpus $(nproc) mytask || exit
docker exec -it debuilder unzip /build/*.zip -d WolkConnect-Cpp  || exit
docker exec -it debuilder /build/make_deb.sh 
docker cp debuilder:$(docker exec debuilder find /build -name '*.deb' | head -n1) .

rm -rf *.zip

docker container stop debuilder
docker container rm debuilder
