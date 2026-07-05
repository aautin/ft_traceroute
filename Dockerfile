FROM debian:bookworm

WORKDIR /workspace

RUN apt-get update && apt-get install -y \
	build-essential \
	git             \
	gdb             \
	bash            \
	net-tools       \
	iputils-ping    \
	iproute2        \
	traceroute      \
	tcpdump         \
	&& rm -rf /var/lib/apt/lists/*

COPY . .

CMD ["bash"]