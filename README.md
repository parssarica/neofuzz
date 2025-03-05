# Installation

Start by cloning this repo:
```
neofuzz@neofuzz:~$ git clone https://github.com/parssarica/neofuzz.git
neofuzz@neofuzz:~$ cd neofuzz
```

Then follow with your OS:

## Linux

### Fedora based distros
In Fedora based distros, You can install dependencies with this command and build it by make:
```
neofuzz@neofuzz:~$ sudo dnf install libcurl-devel gcc make
neofuzz@neofuzz:~$ make && sudo make install
```

### Debian based distros (e.g. Ubuntu, Kali Linux)

In Debian based distros, you can install dependencies with this command and build it by make:
```
neofuzz@neofuzz:~$ sudo apt install libcurl libcurl-devel gcc make
neofuzz@neofuzz:~$ make && sudo make install
```

### Alpine Linux
In Alpine Linux, you can install dependencies with this command and build it by make:
```
neofuzz@neofuzz:~$ sudo apk add curl-dev libcurl musl-dev gcc make
neofuzz@neofuzz:~$ make && sudo make install
```

### Opensuse based distros
In Opensuse based distros, you can install dependencies with this command and build it by make:
```
neofuzz@neofuzz:~$ sudo zypper in libcurl-devel gcc make
neofuzz@neofuzz:~$ make && sudo make install
```

### Arch based distros
In Arch based distros, you can build by make:
```
neofuzz@neofuzz:~$ make && sudo make install
```

## FreeBSD
In FreeBSD, you can install dependencies with this command and build it by make:
```
neofuzz@neofuzz:~$ sudo pkg add curl gcc make
neofuzz@neofuzz:~$ make && sudo make install
```
# Neofuzz Usage

## Basic Usage
To fuzz a url, specify the url with -u/--url and specify the wordlist by -w/--word-list. Here is an example usage:
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ
```

## Matches and filters

You would generally prefer that neofuzz displays only the details of requests that meet the specified filters and matches, rather than displaying every request sent. Here are the available filters and matches:

```
--mc -> Matches the requests that has status codes specified.
--fc -> Filters the requests that has status codes specified.
--ms -> Matches the requests that has content length count specified.
--fs -> Filters the requests that has content length count specified.
--ml -> Matches the requests that has line count specified.
--fl -> Filters the requests that has line count specified.
--mw -> Matches the requests that has word count specified.
--fw -> Filters the requests that has word count specified.
```
Here is an example usage:
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ --mc 200,400
```
This will show requests that has status codes 200 and 404.
Here is another example usage:
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ --ms 1200,1300
```
This will show requests that has content lengths 1200 and 1300.

A more complicated example, if you need to show requests that has content lengths between 1000 and 2000, greater than 3000, smaller than 1500 and equal to 2327. You can just do it by writing:
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ --ms 1000-2000,3000-,-1500,2327
```
This feature is available to --ms, --fs, --mw, --fw, --ml and --fl.

## Sending requests using POST and other types of methods

To send POST requests, you can use -d/--post-data parameter. Here is an example usage:
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/auth -d '{"password": "FUZZ"}' -H "Content-Type: application/json"
```
(The reason for adding the application/json header is that most applications expect the content type to be json.)

Note that it is important to add quotes around the post data; otherwise, the shell will interpret it as separate arguments, and neofuzz will warn you that the arguments were not entered correctly.

The request method doesn't have to be GET or POST. You can specify other request methods with -X/--http-verb options.  Here is an example usage:
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ -X "PUT"
```
This will send a PUT request.

The request method doesn't have to be valid. For example you can send this request method: MeT hOd
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ -X "MeT hOd"
```
## Headers and cookies

It's easy to send headers and cookies. For example, to send two different headers:
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ -H "X-header1: value1" -H "X-header2: value2"
```
This will send the headers we specified.
You can also specify all headers in one header argument by separating them with "\\n":
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ -H "X-header1: value1\nX-header2: value2"
```

To send cookies, you can set the cookie with -b/--cookie parameter. Here's an example:
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ -b "x=y;z=a"
```

Entering -b multi times is not supported. If you enter multiple times, the last entered one will be used.
**Note:** Header and cookie fuzzing isn't supported.

## Proxy

To fuzz using a proxy, you can simply enter the proxy address by -x or --proxy. Here's an example:

```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ -x socks5://1.2.3.4:1234
```

## Timeout

To specify a timeout duration, you can use --timeout option. Here's an example:

```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ --timeout 50
```

The default timeout is 10 seconds.

## User agent

To specify a user agent, you can use --user-agent option. Here's an example:

```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ --user-agent "My User Agent"
```

The default user agent is Windows 10 Chrome user agent. 

## Wait between requests
Sometimes, it is necessary to wait between requests to avoid being blocked by the server. To set the duration for waiting between requests, you can use the --wait option. Here's an example:

```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ --wait 1000
```  

In this example, neofuzz will wait for 1 second after sending each request in **each thread**.
**Note:** You should enter the time as milliseconds. (1 second = 1000 milliseconds.)

## Verbose
If you need to see the request sent and received, you should use verbose mode. To use verbose mode, you can use -v / --verbose parameters.
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ -v
```

## Resolve as
You can instruct neofuzz to resolve a domain as a target IP of your choice using the --resolve-as parameter. Using this feature, you can spoof the domain name or use a non-existent domain name. Here's an example:
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://www.fake-domain.com/FUZZ --resolve-as "www.fake-domain.com:80:1.2.3.4"
```


## Domain Name Fuzzing
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://FUZZ.example.com/
```

## Other Options

### Encode
If you have some strings that is not valid to send in the url or in post data, you can send them by encoding. To encode, you can write "-e" option.
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ -e
```
If you have a word like: "$'''..    abcdef"
Encode will translate it into this and send it: %22%24%27%27%27..%20%20%20%20abcdef%22

### Disable Keep Alive
Sometimes you need to disable keep alive. To do this, you can use --disable-keep-alive option.
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ --disable-keep-alive
```

### Show errors
If you notice that the error count is rising too much, you might want to add the --show-error option to see what these errors are. When you add --show-error, neofuzz will print every error along with the url where the error occurred.
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ --show-error
```

### Poll Mode
If you need neofuzz to use less CPU, you can enable 'Poll Mode'. This mode reduces CPU usage by up to 50%. However, the downside is a slight reduction in performance; in my tests, it decreased performance by approximately 15%. Here's an example of how to enable Poll Mode:
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ --poll-mode
```
## Performance Options

### Parallel sockets

You can specify the number of parallel sockets to open using the -p option. When you specify this number, it is split evenly among the threads indicated by the --num-cpu-cores option. For example, if you set 50 parallel sockets and --num-cpu-cores to 10, then 50 parallel sockets will be equally distributed across 10 cores (5 parallel sockets per core). Here's an example usage with 70 parallel sockets:
```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ -p 70
```

**Note:** The default value of parallel sockets is 50.

### Number of CPU cores

You can specify the number of CPU cores to use via the --num-cpu-cores option, which essentially determines the number of threads to be created. By default, neofuzz detects the number of CPU cores and creates one thread per CPU core. Although it is possible to create more threads than there are CPU cores, doing so will reduce performance. Here's an example:

```
neofuzz@neofuzz:~$ neofuzz -w wordlist_to_use.txt -u http://example.org/FUZZ --num-cpu-cores 16
```

**Note:** In most cases, leaving it at the default setting and using all available cores will yield the best performance.

# Benchmarks

**Client CPU Core Count:** 24 (2 x Intel(R) Xeon(R) E5-2630 v2 (24) @ 3.10 GHz)
**Client RAM:** 64 GB
**Client OS:** Linux Fedora 41
**Client Kernel Version:** 6.12.11

**Server CPU Core Count:** 16 (AMD Ryzen 7800x3D)
**Server RAM:** 64 GB
**Server OS:** Linux Fedora 41
**Server Kernel Version:** 6.12.11
**Web Server:** Nginx

**Network Type:** LAN
**Network speed:** 1Gbit/s

**Wordlist:** rockyou.txt
**User Agent:** Fuzz Faster U Fool v1.0.2

**Fuzzer 1:** neofuzz
**Fuzzer 2:** ffuf

## Multi Core Performance

### 40 parallel sockets / threads (Default value in ffuf)

```
neofuzz@neofuzz:~$ neofuzz -w rockyou.txt -u http://10.0.0.1/FUZZ --user-agent "Fuzz Faster U Fool v1.0.2" -p 40
```
```
ffuf@ffuf:~$ ffuf -w rockyou.txt -u http://10.0.0.1/FUZZ
```

**neofuzz:** 155827 requests per second

**ffuf:** 101016 requests per second

### 100 parallel sockets / threads

```
neofuzz@neofuzz:~$ neofuzz -w rockyou.txt -u http://10.0.0.1/FUZZ --user-agent "Fuzz Faster U Fool v1.0.2" -p 100
```
```
ffuf@ffuf:~$ ffuf -w rockyou.txt -u http://10.0.0.1/FUZZ -t 100
```

**neofuzz:** 213974 requests per second

**ffuf:** 105473  requests per second

### 500 parallel sockets / threads

```
neofuzz@neofuzz:~$ neofuzz -w rockyou.txt -u 10.0.0.1/FUZZ --user-agent "Fuzz Faster U Fool v1.0.2" -p 500
```
```
ffuf@ffuf:~$ ffuf -w rockyou.txt -u http://10.0.0.1/FUZZ -t 500
```

**neofuzz:** 199227 requests per second

**ffuf:** 118548  requests per second


