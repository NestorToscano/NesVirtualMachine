# NesVirtualMachine
The Start of my C/C++ Virtual Machine


docker build -t my_lc3 .

interative terminal + mount images folder
docker run --rm -it -v $(pwd)/images:/app/images my_lc3 ./lc3 /app/images/2048.obj