BambooHR "who is out" Slack bot  
==

BambooSlacking is a Slack bot that's integrated with your BambooHR account 
and can tell who is out today.

Features
--

* Automatically synchronizes current Slack user profile statuses 
for all members of your team based on BambooHR time offs table.
* Gives an information about who is out today by using /whoisout slash command. 

Installation
--

* Create a new Slack application
* Create a new slash command `/whoisout` and set request URL as `https://your.host/command`
* Make sure that "Escape channels, users, and links sent to your app" is enabled.
* Add a new OAuth Redirect URL as `https://your.host/redirect`
* Required permissions scopes: `users.profile:write`, 
`users.profile:read`, 
`users:read.email`,
`users:read`, 
`commands`.
* Download and install binary package for ubuntu-18.04: 

```bash
apt update
apt -y install libboost-all-dev libleveldb-dev
curl -O https://github.com/recipe/bambooslacking/releases/download/v1.1/bambooslacking-1.1-1.bionic_amd64.deb
dpkg -i bambooslacking-1.1-1.bionic_amd64.deb
```

<details><summary>If you want you may also build and install a package from the source code. Click to show example</summary>
<p>

```bash
sudo su -
# download a latest version of cmake
curl -O https://apt.kitware.com/keys/kitware-archive-latest.asc
apt-key add kitware-archive-latest.asc
apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
# install all necessary libraries
apt update
apt -y install cmake libboost-all-dev libleveldb-dev

# in case ninja build fails with cannot allocate memory we have to create a swap file:
fallocate -l 3G /swapfile
chmod 600 /swapfile
mkswap /swapfile
swapon /swapfile
echo '/swapfile none swap sw 0 0' | tee -a /etc/fstab

# we are building cpprestsdk from a source code
apt install -y git libssl-dev ninja-build
cd /usr/src
git clone https://github.com/microsoft/cpprestsdk.git
cd cpprestsdk
git checkout tags/v2.10.16
git submodule update --init
mkdir build.release
cd build.release
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=0
ninja
ninja install

# clone this repo with the source code
cd /usr/src
git clone https://github.com/recipe/bambooslacking.git
# compile application
cd bambooslacking/build/release
cmake ../../
make
cd ../../deb
# build a package
./build.sh
# install a package
dpkg -i bambooslacking_1.1-1.deb
```
</p>
</details>

* Set the config for the application:
```
cp /etc/bambooslacking/config.json-sample /etc/bambooslacking/config.json
```

Change the `server_endpoint` (use the public IP address of your web server).
Provide `slack_client_id`, `slack_client_secret` and `slack_signing_secret` that 
should be obtained while installing the Slack application. 
Use offered randomly generated `cryptokey` or provide your own with the same length (64 characters).
If you want the application to work over the HTTPS, you should generate the SSL certificate.
You may generate either a self-signed certificate or install [Let's encrypt](https://letsencrypt.org/) certificate.

To start the service run the following command:
```
systemctl start bambooslacking
```

Initialization
--
If the application is installed and running you can use the slash command 
`/whoisout help` to see all available options.
 
`/whoisout install <bamboohr_subdomain> <bamboohr_secret>`command is necessary to bind your 
BambooHR account to a Slack workspace. The profile statuses of users that exist in the Slack workspace 
will be updated automatically according to BambooHR's time off table. 
The application matches users by comparing their email addresses.

