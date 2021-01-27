build_profile := release

dockers_dir := dockers
build_dockerfile := $(dockers_dir)/Dockerfile.compile

compile_docker := gym-compile

work_dir := `pwd`
ns_dir := $(work_dir)/ns-3-dev
ns_app := $(work_dir)/ns-app
scratch_webrtc_dir := $(ns_app)/scratch/webrtc_test
ex_webrtc_dir := $(ns_app)/src/ex-webrtc
ex_webrtc_model_dir := $(ex_webrtc_dir)/model
ex_webrtc_wscript_path := $(ex_webrtc_dir)/wscript
alphartc_dir := $(work_dir)/AlphaRTC
alphartc_test_dir := $(alphartc_dir)/test
alphartc_api_dir := $(alphartc_dir)/api
alphartc_modules_dir := $(alphartc_dir)/modules
alphartc_rtc_base_dir := $(alphartc_dir)/rtc_base
target_dir := $(work_dir)/target

docker_user := onl
docker_work_dir := /home/$(docker_user)
docker_alphartc_dir := $(docker_work_dir)/alphartc
docker_ns_dir := $(docker_work_dir)/ns
docker_ns_scratch_webrtc_dir := $(docker_ns_dir)/scratch/webrtc_test
docker_ex_webrtc_dir := $(docker_ns_dir)/src/ex-webrtc
docker_ex_webrtc_model_dir := $(docker_ex_webrtc_dir)/model
docker_ex_webrtc_wscript_path := $(docker_ex_webrtc_dir)/wscript
docker_ex_webrtc_test_dir := $(docker_ex_webrtc_dir)/test
docker_ex_webrtc_api_dir := $(docker_ex_webrtc_dir)/api
docker_ex_webrtc_modules_dir := $(docker_ex_webrtc_dir)/modules
docker_ex_webrtc_rtc_base_dir := $(docker_ex_webrtc_dir)/rtc_base
docker_target_dir := $(docker_ns_dir)/target
docker_ns_build_dir := $(docker_ns_dir)/build/scratch/webrtc_test

docker_flags := --rm \
				-v $(ns_dir):$(docker_ns_dir) \
				-v $(alphartc_dir):$(docker_alphartc_dir) \
				-v $(scratch_webrtc_dir):$(docker_ns_scratch_webrtc_dir) \
				-v $(ex_webrtc_model_dir):$(docker_ex_webrtc_model_dir) \
				-v $(ex_webrtc_wscript_path):$(docker_ex_webrtc_wscript_path) \
				-v $(alphartc_test_dir):$(docker_ex_webrtc_test_dir) \
				-v $(alphartc_api_dir):$(docker_ex_webrtc_api_dir) \
				-v $(alphartc_modules_dir):$(docker_ex_webrtc_modules_dir) \
				-v $(alphartc_rtc_base_dir):$(docker_ex_webrtc_rtc_base_dir) \
				-w $(docker_ns_dir) \
				-e ALPHARTC_DIR=$(docker_alphartc_dir)

all:
	make init
	make sync
	make gym

init:
	docker build dockers --build-arg UID=$(shell id -u) --build-arg GUID=$(shell id -g) -f $(build_dockerfile) -t $(compile_docker)
	git submodule init
	git submodule update

sync:
	make -C $(alphartc_dir) init
	make -C $(alphartc_dir) sync host_workdir=$(work_dir) docker_homedir=/app docker_workdir=/app/AlphaRTC
	make -C $(alphartc_dir) lib

gym:
	mkdir -p $(target_dir)
	docker run $(docker_flags) \
		-v $(target_dir):$(docker_target_dir) \
		$(compile_docker) \
		bash -c \
		" \
		$(docker_ns_dir)/waf configure --enable-static \
		--check-cxx-compiler=clang++ --check-c-compiler=clang \
		--build-profile=$(build_profile); \
		$(docker_ns_dir)/waf build; \
		cp $(docker_ns_dir)/build/scratch/webrtc_test/webrtc_test $(docker_target_dir)/ \
		"

login:
	docker run $(docker_flags) --ulimit core=-1 --security-opt seccomp=unconfined --privileged -ti \
		-v $(target_dir):$(docker_target_dir) \
		$(compile_docker) \
		bash

clean:
	docker run $(docker_flags) $(compile_docker) \
		bash -c \
		" \
		rm -f $(docker_ns_build_dir)/* \
		"
