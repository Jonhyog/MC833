from scapy.all import rdpcap, ls, IP, ICMP

def get_throughput(pcap):
    hosts, throughput = {}, {}

    # Gets first and last timestamp of pkts
    # for each host in pcap
    for pkt in pcap:
        src = pkt[IP].src
        data = hosts.get(src)

        if data:
            hosts[src] = (data[0], pkt, data[2] + len(pkt))
        else:
            hosts[src] = (pkt, None, len(pkt))

    # Calculates avg throughput for each host    
    for host, data in hosts.items():
        throughput[host] = data[2] / (data[1].time - data[0].time)

    return throughput

def get_delay(pcap):
    hosts, delay = {}, {}

    # Gets all timestamps
    # for each host in pcap
    for pkt in pcap:
        src = pkt[IP].src
        data = hosts.get(src)

        if data:
            hosts[src] = hosts[src] + [pkt]
        else:
            hosts[src] = [pkt]

    # Calculates avg delay for each host    
    for host, data in hosts.items():
        pairs = [(data[i - 1], data[i]) for i in range(1, len(data))]
        delay[host] = sum(map(lambda x: x[1].time - x[0].time, pairs)) / len(pairs)

    return delay

def get_count(pcap):
    count = {}

    # Gets all timestamps
    # for each host in pcap
    for pkt in pcap:
        src = pkt[IP].src
        data = count.get(src)

        if data:
            count[src] = count[src] + 1
        else:
            count[src] = 1

    return count

def main():
    raw_pcap = rdpcap("data/h1-h3.pcap")
    pcap = list(filter(lambda x: ICMP in x, raw_pcap))
    
    throughput = get_throughput(pcap)
    delay = get_delay(pcap)
    count = get_count(pcap)

    print(f"{throughput=}")
    print(f"{delay=}")
    print(f"{count=}")
    
    print(f"{len(pcap)=} {len(raw_pcap)=}")

    
if __name__ == "__main__":
    main()