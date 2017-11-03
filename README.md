# DockerRunnerBasic

docker exec (multiple php versions) replacement for myself :D the code quality is not the best (not pretty), but its stable.

## Reason
If you use bunch of docker exec processes with multiple docker machines on the same computer you know the dockerd gets slow and really hard to restart. Or sometimes you cannot even do that.

### docker exec interactive problem
Interactive docker exec is calling the docker http api over tcp/unix socket/etc and keeps the connection alive and sending data to http and receiving data via http. Its just plain stupid.
Other problem is if you have multiple users on the same machine and you dont want to give them docker group access (basically full root access to the machine). But docker exec will always run as root even if you use docker exec -u $uid:$gid. So if it gets stuck the user wont be able to clean that up.

## how it works?
Tt is locating the docker configration, then looks for hostname and pid so it could attach to it with setns() low level calls. (sched.h)

## install & usage
```
root@machine:~# make clean
root@machine:~# make
root@machine:~# chmod +s docker-runner # setuid bit, so it can be used by multiple users on your system. After it attaches to the container it will change its uid back the original users'
root@machine:~# cp docker-runner /somewhere/to/copy

root@machine:~# you can write your own CPHPRunner class, so you can support other commands
root@machine:~# ln -sf /somwhere/docker-runner /usr/bin/php71
root@machine:~# ln -sf /somwhere/docker-runner /usr/bin/php70

root@machine:~# docker ps |grep php7
59955bd12c08        dockerphpreal_php71-php-fpm                           "/usr/sbin/runit_b..."   About an hour ago   Up About an hour                                                                                                                                              php71
48e43e06121f        dockerphpreal_php70-php-fpm                           "/usr/sbin/runit_b..."   About an hour ago   Up About an hour                                                                                                                                                          php70
root@machine:~# su - user
user@machine:~$
user@machine:~$ php71 -v
PHP 7.1.10-1+ubuntu16.04.1+deb.sury.org+1 (cli) (built: Sep 29 2017 17:04:25) ( NTS )
user@machine:~$ php70 -v
PHP 7.0.24-1~dotdeb+8.1 (cli) ( NTS )

user@machine:~$ php70 test2.php &
user@machine:~$ ps auxnfw|grep php70
   12345  8295  0.1  0.0 354320 31276 pts/57   S+   12:57   0:00  |               \_ [php70] /usr/bin/php /www/user/test2.php

```

### Other
If you want to support anything other than multiple php, then you can write your own CPHPRunner class
