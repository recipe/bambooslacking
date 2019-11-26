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
* Download source code, build and install a package (Currently only ubuntu-18.04 is tested)

```bash
# download a latest version of cmake
get -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
# install all necessary libraries
apt update
apt install cmake libcpprest-dev libboost-all-dev libleveldb-dev
# clone this repo with the source code
git clone https://github.com/recipe/bambooslacking.git
# compile application
cd bambooslacking
/usr/bin/cmake -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - Unix Makefiles" ./
/usr/bin/cmake --build build/release --target bambooslacking -- -j 1
cd deb
# build a package
./build.sh
# install a package
dpkg -i bambooslacking_1.0-1.deb
```

* Set config for an application:
```
cp /etc/bambooslacking/config.json-sample /etc/bambooslacking/config.json
```

Change `server_endpoint` (use public IP address of your web server).
Provide `slack_client_id`, `slack_client_secret` and `slack_signing_secret` which 
have been obtained by installing Slack application. 
Use offered randomly generated `cryptokey` or provide your own with the same length (64 characters).
If you want your application works over https you should generate SSL certificate.
You may generate either self signed certificate or install [Let's encrypt](https://letsencrypt.org/) certificate.

To start a service run following command:
```
systemctl start bambooslacking
```

Initialization
--
When application is installed properly and it's running you can use slash command 
`/whoisout help` to see available options.
 
`/whoisout install <bamboohr_subdomain> <bamboohr_secret>` is necessary to bind your 
BambooHR account to a Slack workspace. When it's done successfully profile statuses
of users from the BambooHR account who exist in the Slack workspace will update automatically 
according to BambooHR's time offs table. Users are matched by comparing their 
email addresses.

