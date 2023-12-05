# DNS Server

Simple dns server, that listen on udp port. Support blacklisting based on domain names. 

Build & run:
```console
$ make
$ ./dns_server config_example/test.ini
```

And in a separate terminal
```console
$ host -U -p8080 google.com 127.0.0.1
```

# TODO
- Add ip-based blacklisting
- Support whitelist
- Elegant ways of terminating process


# Credits
ldns - https://nlnetlabs.nl/projects/ldns/about/
inih - https://github.com/benhoyt/inih

