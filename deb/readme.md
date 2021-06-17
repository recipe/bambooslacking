To build a package for ubuntu-18.04 run:

```
sudo ./build.sh
```

To install a package:

```
sudo dpkg -i bambooslacking_1.1-1.deb
```

In case of any dependency error install dependencies with running:

```
sudo apt install -f
```


Then you can start bambooslacking service by running

```
systemctl start bambooslacking 
```