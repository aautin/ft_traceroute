FROM debian:bookworm

WORKDIR /workspace

RUN apt-get update && apt-get install -y \
	build-essential \
	git \
	bash \
	curl \
	wget \
	net-tools \
	iputils-ping \
	iproute2 \
	traceroute \
	&& rm -rf /var/lib/apt/lists/*

COPY . .

CMD ["bash"]