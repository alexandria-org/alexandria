If problem with raid information on drive unmount all partitions and do this:
```
wipefs -a /dev/nvme1n1
```
then reset and install node again.

To setup node with two drives run:
```
source <(curl -s https://raw.githubusercontent.com/alexandria-org/alexandria/main/scripts/bootstrap_node_2drives.sh)
```
