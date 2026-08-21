## Network Interface
### Data format
We shall be sending structs over the network, given that we would have to support encryption in the later phases, it seems like a good idea to serialise the structs and send them over the network for now



```c++
/*
The base packet class properties that will be inherited
*/
struct Packet{
    enum packet_type;
    virtual void serialise(void* ptr);
    virtual void deserialise(void* ptr);
}
```

## Chat Interface

## Repo structure

## Misc
- The commits should be prefixed with their phase ID, for e.g. "PHASE <n>: ..."