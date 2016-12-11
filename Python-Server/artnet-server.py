# Art-Net Server to OSC Message
# The Kazimier Procession
# Inspiration: https://github.com/bbx10/artnet-unicorn-hat
# License: MIT

import time
import logging
import cPickle as pickle
import pprint
from collections import OrderedDict
from math import ceil

from twisted.internet.protocol import DatagramProtocol
from twisted.internet import reactor, threads

from txosc import osc
from txosc import dispatch
from txosc import async

FORMAT = '[%(levelname)s] (%(threadName)-10s) %(message)s'
logging.basicConfig(level=logging.INFO, format=FORMAT)

# Adjust the LED/ brightness as needed.
NODES_ADDR = [72,3,1] #Number of addresses 24 Pixels * 3 bytes + 3 bytes (RGB sigle color) + 1 byte (PWM LED)
NODES_SIZE = sum(NODES_ADDR) #total address for the nodes and address
NODES_MSG = ["RGB","PWM"]
NODES_UNI_SIZE = int(ceil(512.0/NODES_SIZE))
NUM_UNIVERSES = 9
class ArtNet(DatagramProtocol):

    def __init__(self, nodesServer):
        self.nodesServer = nodesServer
    def datagramReceived(self, data, (host, port)):
        if ((len(data) > 18) and (data[0:8] == "Art-Net\x00")):
            rawbytes = map(ord, data)
            opcode = rawbytes[8] + (rawbytes[9] << 8)
            protocolVersion = (rawbytes[10] << 8) + rawbytes[11]
            if ((opcode == 0x5000) and (protocolVersion >= 14)):
                sequence = rawbytes[12]
                physical = rawbytes[13]
                sub_net = (rawbytes[14] & 0xF0) >> 4
                universe = rawbytes[14] & 0x0F
                net = rawbytes[15]
                length = (rawbytes[16] << 8) + rawbytes[17]
#                print "seq %d phy %d sub_net %d uni %d net %d len %d" % (sequence, physical, sub_net, universe, net, length)
                #idx = 18
                #print(rawbytes[18:length+18])
                universeBytes = rawbytes[18:length+18] + (512-length)*[0] #chunks of 512
                threads.deferToThread(self.nodesServer.update, universeBytes, universe)                #checking for all universes before send OSCs packages

class OSCNodesServer(object):
    """
    Example that sends and receives UDP OSC messages.
    """
    def __init__(self):
        self.delay = 0.5
        self.send_host = "127.0.0.1"
        self.send_port = 8888
        self.receive_port = 9999
        try:
            logging.info("Open File with nodes information")
            lFile = open("nodesList.dat", "rb")
            nodes = pickle.load(lFile)
            #Ordered Dictionary with the nodes position
            self.nodesList = OrderedDict(sorted(filter(lambda f:f[1][3] == 0, nodes.iteritems()), key = lambda e:e[1][2]))
            self.nodesListPWM = OrderedDict(sorted(filter(lambda f:f[1][3] > 0, nodes.iteritems()), key = lambda e:e[1][2]))
        except IOError:
            logging.error("File doesn't exist, creating a new one")
            lFile = open("nodesList.dat", "wb")
            pickle.dump({},lFile)
            lFile.close()
            self.nodesList = OrderedDict()
            self.nodesListPWM = OrderedDict()
        pprint.pprint(self.nodesList.items())
        self.rgbBytes = [0]*NUM_UNIVERSES*512 #allocate list
        self.receiver = dispatch.Receiver()
        self.sender = async.DatagramClientProtocol()
        self._sender_port = reactor.listenUDP(0, self.sender)
        self._server_port = reactor.listenUDP(self.receive_port, async.DatagramServerProtocol(self.receiver))

        logging.info("Listening on osc.udp://localhost: {}".format(self.receive_port))

        self.receiver.addCallback("/connect", self.connect)
        self.receiver.addCallback("/alive", self.alive)
        self.receiver.fallback = self.fallback
        #send pings to nodes in case of they're saved in the file
        if not self.nodesList == {}:
          for node, (macAddr, data) in enumerate(self.nodesList.iteritems()): #sort by the time of the conection
                ip = data[0]
                port = self.send_port
                logging.info("Send Alive {} {}".format(ip,macAddr))
                try:
                    self.sender.send(osc.Message("/isAlive", macAddr), (ip,port))
                except:
                    print "Error on ",ip,port
                time.sleep(0.1)
        if not self.nodesListPWM == {}:
          for node, (macAddr, data) in enumerate(self.nodesListPWM.iteritems()): #sort by the time of the conection
                ip = data[0]
                port = self.send_port
                logging.info("Send Alive {} {}".format(ip,macAddr))
                try:
                    self.sender.send(osc.Message("/isAlive", macAddr), (ip,port))
                except:
                    print "Error on ",ip,port
                time.sleep(0.1)

    def update(self, universeBytes, universe):
        self.rgbBytes[universe*512:universe*512 + 512] = universeBytes
        nodes = self.nodesList.items()
        for ind, node in enumerate(nodes[universe*NODES_UNI_SIZE:universe*NODES_UNI_SIZE + NODES_UNI_SIZE]):
            nodeID =  ind + universe*NODES_UNI_SIZE
            nodeChunck = self.rgbBytes[nodeID*NODES_SIZE:nodeID*NODES_SIZE + NODES_SIZE]
            ip = node[1][0]
            port = self.send_port
#            port = data[1]
            oscmsg = osc.Message("/RGB")
            for b in nodeChunck:
                oscmsg.add(b)
            #logging.info("Sending OSC message to {}".format(ip))
            try:
                self.sender.send(oscmsg, (ip,port))
            except:
                pass
                #print "Error on ",ip,port
        if universe == 9:
            nodesPWM = self.nodesListPWM.items()
            lastnPWM, nPWM = 0,0
            for ind, node in enumerate(nodesPWM):
                ip = node[1][0]
                port = self.send_port
                lastnPWM = nPWM
                nPWM = int(node[1][3])
                nodeChunck = universeBytes[ind*lastnPWM:ind*lastnPWM + nPWM]
                oscmsg = osc.Message("/PWMS")
                for b in nodeChunck:
                    oscmsg.add(b)
                #logging.info("Sending OSC message to {}".format(ip))
                try:
                    self.sender.send(oscmsg, (ip,port))
                except:
                    pass


    def alive(self, message, address):
        """
        Alive response form the nodes,
        update port connection, ipaddress
        """
        ip = address[0]
#        port = address[1]
        port = self.send_port
        macAddr = message.getValues()[0]
        logging.info("I'm alive {} {} {}".format(ip,port,macAddr))
        try:
            pwmnode = message.getValues()[1] #in case of a special node, contains num of PWMs on the node
            self.nodesListPWM[macAddr][0:2] = [ip, port]
        except:
            self.nodesList[macAddr][0:2] = [ip, port]

    def connect(self, message, address):
        """
        Method handler for /connect, new node
        """
        print("Got %s from %s" % (message, address))
        ip = address[0]
#        port = address[1]
        port = self.send_port
        macAddr = message.getValues()[0]
        try:
            pwmnode = message.getValues()[1] #in case of a special node, contains num of PWMs on the node
            self.nodesListPWM[macAddr] = [ip, port, time.time(),pwmnode]
        except:
            self.nodesList[macAddr] = [ip, port, time.time(), 0]
        #update ip and port
        self.nodesList = OrderedDict(sorted(self.nodesList.iteritems(), key = lambda e:e[1][2]))
        self.nodesListPWM = OrderedDict(sorted(self.nodesListPWM.iteritems(), key = lambda e:e[1][2]))
        #pprint.pprint(self.nodesList[macAddr])
        try:
            self.sender.send(osc.Message("/connected"), (ip, port))
        except:
            print "Error on ",ip,port


    def fallback(self, message, address):
        """
        Fallback for everything else we get.
        """
        print("Lost ball %s from %s" % (message, address))

    def shutdown(self):
        #send disconnect command do nodes
        lFile = open("nodesList.dat", "wb")
        pickle.dump(self.nodesList, lFile)
        lFile.close()
        #send disconnect in ordered way
        for node, (macAddr, data) in enumerate(self.nodesList.iteritems()): #sort by the time of the conection
            ip = data[0]
            port = self.send_port
            logging.info("Send disconnect {} {}".format(ip,macAddr))
            try:
                self.sender.send(osc.Message("/disconnect", True), (ip,port))
            except:
                print "Error on ",ip,port
            time.sleep(0.1)
        for node, (macAddr, data) in enumerate(self.nodesListPWM.iteritems()): #sort by the time of the conection
            ip = data[0]
            port = self.send_port
            logging.info("Send disconnect {} {}".format(ip,macAddr))
            try:
                self.sender.send(osc.Message("/disconnect", True), (ip,port))
            except:
                print "Error on ",ip,port
            time.sleep(0.1)

if __name__ == "__main__":


    oscServer = OSCNodesServer()
    artNet =  ArtNet(oscServer)
    reactor.listenUDP(6454, artNet)
    reactor.addSystemEventTrigger('before', 'shutdown', oscServer.shutdown)
    reactor.run()
