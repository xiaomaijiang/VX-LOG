# What is VX-Log

VX-LOG is a data acquisition client based on Nxlog,and it will has some useful feature where NXLOG community version not has,such as line-number info for `im-file` module. and `im_kafka` and `om_kafka` module

# VX-Log Features

## File Line Number

line-number Info:line-number is a useful info when we collect data from file.we can use line-number info to do something like `search log arround n line`

## Improve im_mark Info

> make sure sigar is installed

```
git clone --depth=1 https://github.com/hyperic/sigar.git
./autogen.sh && ./configure && make CFLAGS=-fgnu89-inline && sudo make install
```

im_mark Input Module used to monitor agent status,But Nxlog provide info is not
 enough,so VX-LOG add VX-LOG cpu usage and memory usage 

|Fields|Desc|
|----|----|
|MemoryUsage|VX-LOG Memory Usage|
|CpuUsage|VX-LOG Cpu Usage| 
|SystemName|Current System Name|
|SystemVersion|Current System Version|

## Kafka Input&&Kafka Output

Thanks `https://github.com/filipealmeida/nxlog-kafka-output-module` provide Kafka Output Module,VX-Log integrate nxlog-kafka-output-module default.

Download and install librdkafka,and Compile VX-LOG
```
	git clone https://github.com/edenhill/librdkafka.git
	cd librdkafka
	./configure
	make
	sudo make install
```
### Config Sample

Kafka Output

```
	<Output outKafka>
	  Module      om_kafka
	  BrokerList  localhost:9092,otherhost.example.com:9092
	  Topic       test
	  #-- Partition   <number> - defaults to RD_KAFKA_PARTITION_UA
	  #-- Compression, one of none, gzip, snappy
	  Compression gzip
	</Output>
```

Kafka Input
```
<Input im_kafka>
    Module  im_kafka
	BrokerList  192.168.2.248:9092
	Topic       test
	Compression gzip
    Option group.id vxnlog
	Partition   4    #default is 0
</Input>
```


# Some Useful feature wait for implement

* Multi SQL for Database Input
* Fake Template for testgen Input
