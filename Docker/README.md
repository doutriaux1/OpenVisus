# Simple container

Compile docker:

```
# change the name as you need
DOCKER_TAG=openvisus-trusty
sudo docker build --tag $DOCKER_TAG Docker/trusty
```

# Mod visus

Compile docker:

```
DOCKER_TAG=mod_visus-trusty
docker build --tag $DOCKER_TAG \
	--build-arg DISABLE_OPENMP=0 \
	
	Docker/trusty

ARG =0
ARG VISUS_GUI=1
ARG VISUS_MODVISUS=0

```

Configure datasets and run docker:

```
# change this to point to where your visus datasets are stored
VISUS_DATASETS=$(pwd)/datasets

DOCKER_OPTS=""
DOCKER_OPTS+=" -it"  # allocate a tty for the container process.
DOCKER_OPTS+=" --rm" #automatically clean up the container and remove the file system when the container exits
DOCKER_OPTS+=" -v $VISUS_DATASETS:/mnt/visus_datasets" # mount the volume
DOCKER_OPTS+=" -p 8080:80" # map network ports
DOCKER_OPTS+=" --expose=80" # expose the port
docker run $DOCKER_OPTS $DOCKER_TAG "/usr/local/bin/httpd-foreground.sh"

# docker run $DOCKER_OPTS --entrypoint=/bin/bash $DOCKER_TAG
# /usr/local/bin/httpd-foreground.sh
```

To test docker container, in another terminal:

```
curl -v "http://0.0.0.0:8080/mod_visus?action=list"
```

Deploy to the repository:

```
sudo docker login -u scrgiorgio
# TYPE the secret <password>

docker tag $DOCKER_TAG visus/$DOCKER_TAG
docker push visus/$DOCKER_TAG
```