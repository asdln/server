# alg-base Dockerfile
# Version 1.0

# 算法基础镜像
FROM osgeo/gdal:ubuntu-small-latest

ENV LANG C.UTF-8
ENV HOME /usr/local/image_server_quick
##设置docker容器的时间
ENV TZ=Asia/Shanghai
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN mkdir -p ${HOME}

ENV LD_LIBRARY_PATH ${HOME}/release

ADD release.tar.gz ${HOME}/
# COPY  application.properties   ${HOME}/application.properties
# COPY  log4j.properties   ${HOME}/log4j.properties

RUN mkdir -p ${HOME}/release/log

#WORKDIR 相当于cd
WORKDIR ${HOME}/release

#EXPOSE 映射端口
#EXPOSE 19090

#启动执行
#ENTRYPOINT ["./AlgorithmExecutor","Batch_S1_GRD_FastOrthoCorrection"]
