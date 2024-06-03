from scapy.all import rdpcap, ls, IP, ICMP
from decimal import Decimal
from typing import Dict, List

class ICMPMetrics:
    def __init__(self, raw_pcap):
        self.raw_pcap = raw_pcap
        self.prc_pcap = list(filter(lambda x: ICMP in x, raw_pcap))
    
    def get_throughput(self) -> Dict[str, Decimal]:
        hosts, throughput = {}, {}

        # Gets first and last timestamp of pkts
        # for each host in pcap
        for pkt in self.prc_pcap:
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

    def get_delay(self) -> Dict[str, Decimal]:
        hosts, delay = {}, {}

        # Gets all timestamps
        # for each host in pcap
        for pkt in self.prc_pcap:
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

    def get_count(self) -> Dict[str, int]:
        count = {}

        # Gets number of pkts sent by each host
        for pkt in self.prc_pcap:
            src = pkt[IP].src
            data = count.get(src)

            if data:
                count[src] = count[src] + 1
            else:
                count[src] = 1

        return count

    def get_hosts(self) -> List[str]:
        return list(set(map(lambda pkt: pkt[IP].src, self.prc_pcap)))

def main():
    FILE = "data/h1-h3.pcap"
    raw_pcap = rdpcap(FILE)
    metrics = ICMPMetrics(raw_pcap)
    data = {
        "throughput": (metrics.get_throughput(), "B/s"),
        "delay": (metrics.get_delay(), "s"),
        "count": (metrics.get_count(), ""),
        "hosts": (metrics.get_hosts(), ""),
    }

    print(f"Analysing file {FILE}:")
    for key, value in data.items():
        print(f"Calculating {key}:")

        value, unit = value
        if isinstance(value, dict):
            print("\n".join([f"\t{dk}: {dv:.4f} {unit}" for dk, dv in value.items()]))
        else:
            print("\n".join([f"\t{x}" for x in value]))

    # print(f"{len(pcap)=} {len(raw_pcap)=}")

    
if __name__ == "__main__":
    main()