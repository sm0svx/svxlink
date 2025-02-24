

# build docker images used for building linux binary
```
docker build -t svx_build .
````

# run 
```
docker run -it -v "$(pwd)"/svxlink/:/svxlink/ svx_build
```