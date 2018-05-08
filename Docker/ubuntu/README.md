
TO compile Docker:

	docker build -t mod_visus-ubuntu .

To run Docker after compilation:

	docker run -it --rm -p 8080:80 mod_visus-ubuntu 

If you want to debug the docker container:

	docker run -it --rm -p 8080:80 --entrypoint=/bin/bash mod_visus-ubuntu
	/usr/local/bin/httpd-foreground.sh

To test docker container, in another terminal:

	curl -v "http://0.0.0.0:8080/mod_visus?action=readdataset&dataset=cat"


