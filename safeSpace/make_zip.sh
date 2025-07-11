#!/usr/bin/env bash

mkdir -p ./tmp-safe
cd ./tmp-safe || exit
rsync -av --exclude-from=../../.gitignore --exclude='.git' --exclude=$tmp_safe ../../../WolkConnect-Cpp .
cd ./WolkConnect-Cpp || exit
filename="WolkConnect-Cpp-v$(cat RELEASE_NOTES.txt | grep "**Version" | head -1 | sed -e "s/**Version //" | sed -e "s/\*\*//").zip"
echo "filename: $filename"
zip -qr $filename *
mv "$filename" ../..
cd ../..
rm -rf ./tmp-safe
