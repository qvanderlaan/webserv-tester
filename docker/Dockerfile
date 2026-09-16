FROM silkeh/clang:19

RUN apt-get update && \
	apt-get install -y --no-install-recommends \
	libc++-19-dev \
	libc++abi-19-dev && \
	rm -rf /var/lib/apt/lists/*
