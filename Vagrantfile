# -*- mode: ruby -*-
# vi: set ft=ruby :

### Configuration options ###
SRC_SSH_PUBLIC_KEY = "~/.ssh/id_rsa.pub"
BOX_HOST_NAME = "bambooslacking"
BOX_CPUS = 2
BOX_MEMORY = 3072

# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure("2") do |config|
  # The most common configuration options are documented and commented below.
  # For a complete reference, please see the online documentation at
  # https://docs.vagrantup.com.

  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://vagrantcloud.com/search.
  config.vm.box = "ubuntu/bionic64"

  # Disable automatic box update checking. If you disable this, then
  # boxes will only be checked for updates when the user runs
  # `vagrant box outdated`. This is not recommended.
  # config.vm.box_check_update = false

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # accessing "localhost:8080" will access port 80 on the guest machine.
  # NOTE: This will enable public access to the opened port
  # config.vm.network "forwarded_port", guest: 80, host: 8080

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine and only allow access
  # via 127.0.0.1 to disable public access
  # config.vm.network "forwarded_port", guest: 80, host: 8080, host_ip: "127.0.0.1"

  # Create a private network, which allows host-only access to the machine
  # using a specific IP.
  config.vm.network "private_network", ip: "192.168.33.10"

  # Create a public network, which generally matched to bridged network.
  # Bridged networks make the machine appear as another physical device on
  # your network.
  # config.vm.network "public_network"

  # Share an additional folder to the guest VM. The first argument is
  # the path on the host to the actual folder. The second argument is
  # the path on the guest to mount the folder. And the optional third
  # argument is a set of non-required options.
  # config.vm.synced_folder "../data", "/vagrant_data"
  config.vm.synced_folder ".", "/vagrant", disabled: true
  config.vm.synced_folder "./deb", "/deb"

  # Provider-specific configuration so you can fine-tune various
  # backing providers for Vagrant. These expose provider-specific options.
  # Example for VirtualBox:

  config.vm.provider "virtualbox" do |vb|
    ## Display the VirtualBox GUI when booting the machine
    #vb.gui = true

    # Customize VM name
    vb.name = BOX_HOST_NAME

    # Customize cpuz count
    vb.cpus = BOX_CPUS

    # Customize the amount of memory on the VM:
    vb.memory = BOX_MEMORY
  end

  config.vm.provision :hostmanager
  config.hostmanager.enabled = true
  config.hostmanager.manage_host = true
  config.hostmanager.ignore_private_ip = false
  config.hostmanager.include_offline = true
  config.vm.hostname = BOX_HOST_NAME

  config.vm.provision :file, source: SRC_SSH_PUBLIC_KEY, destination: "/home/vagrant/.ssh/id_rsa.pub"

  # Enable provisioning with a shell script. Additional provisioners such as
  # Puppet, Chef, Ansible, Salt, and Docker are also available. Please see the
  # documentation for more information about their specific syntax and use.
  config.vm.provision "shell", args: [BOX_HOST_NAME], inline: <<-SHELL
    OPT_HOSTNAME=$1

    echo -e "\n-- SSH KEY Provisioning --\n"
    cat /home/vagrant/.ssh/id_rsa.pub >> /home/vagrant/.ssh/authorized_keys
    cat /home/vagrant/.ssh/id_rsa.pub >> /root/.ssh/authorized_keys

    echo -e "\n-- Add a package for cmake"
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
    sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'

    echo -e "\n-- Updating packages --\n"
    apt-get update && apt-get -y dist-upgrade

    echo -e "\n--- Setting timezone ---\n"
    timedatectl set-timezone Europe/Kiev
    dpkg-reconfigure --frontend noninteractive tzdata

    echo -e "\n-- Installing dependencies --\n"
    apt-get install -y ntpdate cmake gdb libcpprest-dev libboost-all-dev libleveldb-dev \
    libgtest-dev valgrind

    echo -e "\n-- Installing googletest --\n"
    wget https://github.com/google/googletest/archive/release-1.7.0.tar.gz
    tar xf release-1.7.0.tar.gz
    cd googletest-release-1.7.0
    sudo cmake -DBUILD_SHARED_LIBS=ON .
    sudo make
    sudo cp -a include/gtest /usr/include
    sudo cp -a libgtest_main.so libgtest.so /usr/lib/

    echo -e "\n-- Generating SSL self-signed certificate --\n"
    openssl req -newkey rsa:2048 -new -nodes -x509 -sha256 -days 3650 \
    -keyout /etc/ssl/private/key.pem -out /etc/ssl/private/cert.pem \
    -subj "/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=$OPT_HOSTNAME"
  SHELL
end
