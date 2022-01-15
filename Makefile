DOCKER_IMAGE = sandbox-x86-64

.PHONY: all
all:
	$(MAKE) 9cc -C cc
	$(MAKE) cpu -C cpu

.PHONY: test
test:
	$(MAKE) test -C cc
	$(MAKE) test -C cpu

.PHONY: clean
clean:
	$(MAKE) clean -C cc
	$(MAKE) clean -C cpu
	rm tmp.* emulator.log || true

.PHONY: docker-build
docker-build:
	docker build -t $(DOCKER_IMAGE) .

.PHONY: docker-test
docker-test:
	make clean
	docker run --rm -v `PWD`:/sandbox -w /sandbox $(DOCKER_IMAGE) make test

.PHONY: docker-shell
docker-shell:
	docker run -it --rm -v `PWD`:/sandbox -w /sandbox $(DOCKER_IMAGE) bash
