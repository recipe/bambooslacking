BambooSlacking  
==

This project has been **superseded** by [bamboohr-slack-bot](https://github.com/recipe/bamboohr-slack-bot), and won't receive updates or bug fixes.

----

**BambooSlacking** is a Slack bot that's integrated with your BambooHR account 
and can tell who is out today.

Features
--

* Automatically synchronizes current Slack user profile statuses 
for all members of your team based on BambooHR's time off table.
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
curl -O https://github.com/recipe/bambooslacking/releases/download/v1.2/bambooslacking-1.2-1.bionic_amd64.deb
dpkg -i bambooslacking-1.2-1.bionic_amd64.deb
```

<details><summary>You also may build a package from the source code. Click to show example</summary>
<p>

Build and run a docker container
```bash
git clone https://github.com/recipe/bambooslacking.git
cd bambooslacking
docker build -t bambooslacking:latest . 
export WORKDIR=/usr/src/bambooslacking
docker run -it --rm --name builder -w "$WORKDIR" -v "$(pwd)":"$WORKDIR" --tmpfs /tmp bambooslacking /bin/bash
```
Execute the following inside the container:
```bash
rm -fr build && mkdir -p build/release
cd build/release
cmake ../..
make
cd ../..
# build the package
./deb/build.sh
# to install the package
PKG=$(ls ./build/bambooslacking*.deb)
dpkg -i "$PKG"
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

