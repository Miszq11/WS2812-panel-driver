FROM ubuntu:latest

# insert all downloadable packages here with: apt-get -y install package-name
RUN apt-get update && apt-get -y install sed make binutils build-essential diffutils gcc g++ bash patch gzip bzip2 perl tar cpio unzip rsync file bc findutils wget python3 libncurses5 libncurses5-dev && rm -rf /var/lib/apt/lists/*