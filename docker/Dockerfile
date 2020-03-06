FROM ubuntu:18.04

ADD ./prepare.sh /
RUN ["/bin/bash", "-c", "/prepare.sh"]

ADD ./install.sh /
RUN ["/bin/bash", "-c", "/install.sh"]
