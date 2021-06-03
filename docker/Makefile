platforms=win mac lin
container=blufx-cross-compile-env

# all: mac lin win
# all: mac lin win
all: mac win lin

.image: Dockerfile build.sh
	docker build . -t $(container)
	docker inspect -f "{{ .ID}}" $(container):latest > .image

bash: .image
	docker run -it --rm -v "$(realpath ../):/src" --entrypoint bash $(container):latest

$(platforms): .image
	docker run -i --rm -v "$(realpath ../):/src" $(container):latest $@

.PHONY: $(platforms) bash
