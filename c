package rmi;

import java.rmi.registry.LocateRegistry;
import java.rmi.registry.Registry;

import static common.Utils.*;
import common.MessageInfo;

public class RMIClient {

    public static void main(String[] args) {

        // check arguments
        if (args.length < 3) {
            handleError("Args: server name/IP, recv port, "
                            + "msg count");
        }

        // get total number of messages to send
        int total = getInt(args[2], "total number of messages"
                        + "must be integer");

        // get port number
        int portNum = getInt(args[1], "invalid port number");
        assert (portNum >= 1024 && portNum <= 65535)
                : "port number must be within 1024 to 65535";

        // set up Security Policy
        if (System.getSecurityManager() == null) {
            System.setSecurityManager(new SecurityManager());
        }

        try {
            // locate registry
            Registry registry = LocateRegistry.getRegistry(args[0],
                            portNum);
            // get server and bind to remote object
            RMIServerInterface server = (RMIServerInterface) registry
                            .lookup("RMIServerInterface");

            // main loop send messages
            for (int i = 0; i < total; i++) {
                MessageInfo message = new MessageInfo(total, i);
                server.receiveMessage(message);
            }

            // finish
            System.out.println(total + " Messages Sent");
            return;
        } catch (Exception e) {
            handleError("error starting client", e);
        }
    }
}





import static common.Utils.concatListInterval;

import java.rmi.RemoteException;
import java.rmi.registry.LocateRegistry;
import java.rmi.registry.Registry;
import java.rmi.server.UnicastRemoteObject;
import java.util.LinkedList;

import static common.Utils.*;
import common.MessageInfo;

public class RMIServer extends UnicastRemoteObject
                implements RMIServerInterface {
    // RMI server implementation

    private int portNum;
    private int total = -1;
    private short[] receivedMsg;
    private int received = -1;

    public RMIServer(int portNum) throws RemoteException {
        super();
        this.portNum = portNum;
    }

    public static void main(String[] args) {

        // check arguments
        if (args.length < 1) {
            handleError("argument required: port number");
        }

        // get port number
        int portNum = getInt(args[0], "invalid port number");
        assert (portNum >= 1024 && portNum <= 65535)
                : "port number must be within 1024 to 65535";

        // set up Security Policy
        if (System.getSecurityManager() == null) {
            System.setSecurityManager(new SecurityManager());
        }

        // setup server
        try {
            // create server
            RMIServerInterface server = new RMIServer(portNum);
            try {
                // locate registry
                Registry registry = LocateRegistry
                                .createRegistry(server.getPortNum());
                // bind server object to remote interface
                registry.rebind("RMIServerInterface", server);
            } catch (RemoteException e) {
                handleError("error binding to server", e);
            }
            System.out.println("Server ready...");
        } catch (Exception e) {
            handleError("RMIServerInterface exception: ", e);
        }
    }

    // call back for receiving a message
    public void receiveMessage(MessageInfo msg)
                    throws RemoteException {

        // setup if first message
        if (received == -1) {
            total = msg.totalMessages;
            receivedMsg = new short[total];
            received = 0;
        }

        assert (total == msg.totalMessages)
                : "inconsistent total message count";

        // update message count
        if (receivedMsg[msg.messageNum] == 0) {
            received++;
        }
        receivedMsg[msg.messageNum] += 1;

        // print summary after last message
        if (msg.messageNum == msg.totalMessages - 1) {
            finish();
        }

    }

    // function to show port number used
    public int getPortNum() {
        return portNum;
    }

    // print summary
    public void finish() {

        System.out.println("finishing");

        // no message received
        if (received == -1) {
            System.out.println("no messages recieved!");
            return;
        }

        LinkedList<Integer> lostMsg = new LinkedList<Integer>();
        LinkedList<Integer> duplicateMsg = new LinkedList<Integer>();

        // collate lost and duplicate messages
        for (int i = 0; i < total; i++) {
            if (receivedMsg[i] == 0) {
                lostMsg.add(i);
            } else if (receivedMsg[i] >= 2) {
                duplicateMsg.add(i);
            }
        }

        // print summary
        System.out.println("Msg sent: " + total);
        System.out.print("Msg received: " + received);
        System.out.println("    of which " + duplicateMsg.size()
                        + " are duplicates");
        System.out.println("Percentage duplicates: "
                        + ((float) duplicateMsg.size()) / received
                                        * 100);
        assert (total - received == lostMsg
                        .size()) : "incosistent message count";
        System.out.println("Msg lost: " + (total - received));
        System.out.println("Percentage lost: "
                        + ((float) (total - received))
                        / total * 100);

        // print details about lost and duplicate message numbers
        if (lostMsg.size() > 0) {
            System.out.println("lost messages:");
            System.out.println(concatListInterval(lostMsg));
        }
        if (duplicateMsg.size() > 0) {
            System.out.println("duplicated messages:");
            System.out.println(concatListInterval(duplicateMsg));
        }
    }

}




package udp;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;

import static common.Utils.*;
import common.MessageInfo;

public class UDPClient {
    // UDP client implementation

    private DatagramSocket sendSoc;

    public UDPClient() {
        try {
            // initialise socket
            sendSoc = new DatagramSocket();
        } catch (SocketException e) {
            handleError("error initialising socket", e);
        }
    }

    public static void main(String[] args) {
        InetAddress serverAddr = null;
        int portNum;
        int total;

        // check arguments
        if (args.length < 3) {
            handleError("arguments: hostname, port number, "
                            + "msg count");
        }

        // get server address
        try {
            serverAddr = InetAddress.getByName(args[0]);
        } catch (UnknownHostException e) {
            handleError("unknown host exception at server, "
                            + args[0], e);
        }

        // get port number
        portNum = getInt(args[1], "invalid port number");
        assert (portNum >= 1024 && portNum <= 65535)
                : "port number must be within 1024 to 65535";

        // get total number messages to send
        total = getInt(args[2], "total msg must be integer");

        // create client
        UDPClient client = new UDPClient();

        // run client
        client.run(serverAddr, portNum, total);
        return;
    }

    // client main loop
    public void run(InetAddress serverAddr, int portNum, int total) {
        System.out.println("client ready...");

        for (int i = 0; i < total; i++) {
            // create message
            MessageInfo msg = new MessageInfo(total, i);

            // send message
            this.send(msg.toString(), serverAddr, portNum);
        }
        // finish sending
        System.out.println(total + " Messages sent");
    }

    // function to send message
    private void send(String payload, InetAddress destAddr,
                    int destPort) {

        // create packet
        byte[] pktData = payload.getBytes();
        int payloadSize = pktData.length;
        DatagramPacket pkt;
        pkt = new DatagramPacket(pktData, payloadSize, destAddr,
                        destPort);

        try {
            // send the packet over the socket
            sendSoc.send(pkt);
        } catch (IOException e) {
            handleError("error sending packet");
        }
    }
}





package udp;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;
import java.util.LinkedList;

import static common.Utils.*;
import common.MessageInfo;

public class UDPServer {
    // UDP server implementation

    private final static int BUFFERSIZE = 50;
    private final static int TIMEOUT = 40000;
    private DatagramSocket socket;
    private int total = -1;
    private short[] receivedMsg;
    private int received = -1;

    public UDPServer(int portNum) {

        try {
            // open socket and listen for messages
            socket = new DatagramSocket(portNum);
        } catch (SocketException e) {
            handleError("error initialising socket", e);
        }
        System.out.println("Server ready...");
    }

    public static void main(String args[]) {

        // check arguments
        if (args.length < 1) {
            handleError("argument required: port number");
        }

        // get port number
        int portNum = getInt(args[0], "port number must be integer");
        assert (portNum >= 1024 && portNum <= 65535)
                : "port number must be within 1024 to 65535";

        // setup server
        UDPServer server = new UDPServer(portNum);

        // run server
        server.run();

        // output results
        server.finish();
        return;
    }

    // server main loop
    private void run() {

        byte[] pacData = new byte[BUFFERSIZE];
        DatagramPacket pac = null;
        boolean open = true;

        // receive message until all received == total or timeout
        do {
            try {

                // create buffer
                pac = new DatagramPacket(pacData, BUFFERSIZE);

                // set timeout on Socket
                socket.setSoTimeout(TIMEOUT);

                // receive message
                socket.receive(pac);

                // process message
                process(new String(pac.getData()).trim());
            } catch (IOException e) {
                // timeout finishes
                open = false;
            }
        } while (open && total != received);
    }

    // process message
    private void process(String data) {

        // parse string to MessageInfo
        MessageInfo msg = null;
        try {
            msg = new MessageInfo(data);
        } catch (Exception e) {
            handleError("invalid data format", e);
        }

        // setup if first message
        if (received == -1) {
            total = msg.totalMessages;
            receivedMsg = new short[total];
            received = 0;
        }

        assert (total == msg.totalMessages)
                : "inconsistent total number of messages";

        // update message count
        if (receivedMsg[msg.messageNum] == 0) {
            received++;
        }
        receivedMsg[msg.messageNum] += 1;
    }

    // print summary
    public void finish() {

        System.out.println("finishing");

        // no message received
        if (received == -1) {
            System.out.println("no messages recieved!");
            return;
        }

        LinkedList<Integer> lostMsg = new LinkedList<Integer>();
        LinkedList<Integer> duplicateMsg = new LinkedList<Integer>();

        // collate lost and duplicate messages
        for (int i = 0; i < total; i++) {
            if (receivedMsg[i] == 0) {
                lostMsg.add(i);
            } else if (receivedMsg[i] >= 2) {
                duplicateMsg.add(i);
            }
        }

        // print summary
        System.out.println("Msg sent: " + total);
        System.out.print("Msg received: " + received);
        System.out.println("    of which " + duplicateMsg.size()
                        + " are duplicates");
        System.out.println("Percentage duplicates: "
                        + ((float) duplicateMsg.size()) / received
                                        * 100);
        assert (total - received == lostMsg
                        .size()) : "incosistent message count";
        System.out.println("Msg lost: " + (total - received));
        System.out.println("Percentage lost: "
                        + ((float) (total - received))
                        / total * 100);

        // print details about lost and duplicate message numbers
        if (lostMsg.size() > 0) {
            System.out.println("lost messages:");
            System.out.println(concatListInterval(lostMsg));
        }
        if (duplicateMsg.size() > 0) {
            System.out.println("duplicated messages:");
            System.out.println(concatListInterval(duplicateMsg));
        }
    }

}




Other classes:

package rmi;

import java.rmi.Remote;
import java.rmi.RemoteException;

import common.*;

public interface RMIServerInterface extends Remote {
	// interface for remote object
	
	public void receiveMessage(MessageInfo msg)
	    throws RemoteException;

	public int getPortNum() throws RemoteException;
}

package common;

import java.util.LinkedList;

public class Utils {
	// common functionality

	public static final String dash = " - ";
	public static final String sep = ", ";

	// function to collate intervals in a list
	public static String concatListInterval(LinkedList<Integer>
	                                        list) {
		if (list == null || list.size() == 0) {
			return "";
		}

		StringBuilder sb = new StringBuilder();

		while (list.size() > 0) {
			int start = list.poll();
			int end = start;
			while (list.size() > 0 && list.peek() == end + 1) {
				end = list.poll();
			}
			if (start != end) {
				sb.append(String.valueOf(start));
				sb.append(dash);
				sb.append(String.valueOf(end));
			} else {
				sb.append(String.valueOf(start));
			}
			if (list.size() > 0) {
				sb.append(sep);
			}
		}

		return sb.toString();
	}

	// functions for handling error
	public static void handleError(String err) {
		System.err.println(err);
		System.exit(-1);
	}

	public static void handleError(String err, Exception e) {
		System.err.println(err);
		e.printStackTrace();
		System.exit(-1);
	}

	// function to parse integer
	public static int getInt(String s, String err) {
		int result = -1;
		try {
			result = Integer.parseInt(s);
		} catch (NumberFormatException e) {
			handleError(err, e);
		}
		return result;
	}
}
