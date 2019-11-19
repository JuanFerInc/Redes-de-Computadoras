import java.util.HashMap;
import java.util.HashSet;
import java.util.Map.Entry;

public class RouterNode {
	private int myID;
	private GuiTextArea myGUI;
	private RouterSimulator sim;
	private HashMap<Integer, Integer> neighbors;
	private HashMap<Integer, HashMap<Integer, Integer>> matriz;
	HashMap<Integer, Integer> fordwarding;
	public static final int INFINITY = 999;

	// --------------------------------------------------
	public RouterNode(int ID, RouterSimulator sim, HashMap<Integer, Integer> costs) {
		myID = ID;
		matriz = new HashMap<Integer, HashMap<Integer, Integer>>();
		this.sim = sim;
		myGUI = new GuiTextArea("  Output window for Router #" + ID + "  ");
		this.neighbors = (HashMap<Integer, Integer>) costs.clone();
		neighbors.put(INFINITY, 1);
		for (Integer i : neighbors.keySet()) {
			if (i != INFINITY) {
				RouterPacket pkt = new RouterPacket(myID, i, neighbors);
				sendUpdate(pkt);
			}
		}
		neighbors.put(INFINITY, 2);
	}

	private void dijktra() {
		fordwarding = new HashMap<Integer, Integer>();
		HashMap<Integer,Integer> explorados = new HashMap<Integer,Integer>();
		explorados.put(myID,0);
		HashMap<Integer,Integer> costos = new HashMap<Integer,Integer>();
		for(Integer i: matriz.keySet()) {
			if(neighbors.containsKey(i)) {
				costos.put(i,neighbors.get(i));
				fordwarding.put(i, i);
			}else {
				costos.put(i, INFINITY);
			}
			
		}
		while(true) {
			Entry<Integer,Integer> min = null;
			for (Entry<Integer, Integer> entry : costos.entrySet()) {
			    if (min == null || min.getValue() > entry.getValue()) {
			        min = entry;
			    }
			}
			if(min == null || min.getValue() == INFINITY)
				break;
			explorados.put(min.getKey(),min.getValue());
			costos.remove(min.getKey());
			for(Integer i: matriz.get(min.getKey()).keySet()) {
				if(costos.containsKey(i) && costos.get(i) > min.getValue() + matriz.get(min.getKey()).get(i)) {
					costos.put(i, min.getValue() + matriz.get(min.getKey()).get(i));
					fordwarding.put(i,fordwarding.get(min.getKey()));
				}
			}
		}
		for(int i: costos.keySet()) {
			matriz.remove(i);
		}
		
		
		printDistanceTable();
	}

	// --------------------------------------------------
	public void recvUpdate(RouterPacket pkt) {
		boolean flooding = false;
		Integer a, b;
		a = pkt.mincost.get(INFINITY);
		if (matriz.get(pkt.sourceid) == null) {
			for (Integer i : neighbors.keySet()) {
				if (i != INFINITY && i != pkt.sourceid) {
					RouterPacket pkt2 = new RouterPacket(pkt.sourceid, i, pkt.mincost);
					sendUpdate(pkt2);
				}
			}
			matriz.put(pkt.sourceid, pkt.mincost);
			dijktra();
			flooding = true;
		} else {
			b = matriz.get(pkt.sourceid).get(INFINITY);
			if (a > b) {
				for (Integer i : neighbors.keySet()) {
					if (i != INFINITY && i != pkt.sourceid) {
						RouterPacket pkt2 = new RouterPacket(pkt.sourceid, i, pkt.mincost);
						sendUpdate(pkt2);
					}
				}
				matriz.put(pkt.sourceid, pkt.mincost);
				dijktra();
			}
		}
		/*
		if (neighbors.containsKey(pkt.sourceid) && pkt.mincost.get(myID) != neighbors.get(pkt.sourceid)) {
			neighbors.put(pkt.sourceid, pkt.mincost.get(myID));
			flooding = true;
		}
		*/
		
		if (flooding) {
			for (Integer i : neighbors.keySet()) {
				if (i != INFINITY) {
					RouterPacket pkt3 = new RouterPacket(myID, i, neighbors);
					sendUpdate(pkt3);
				}
			}
			neighbors.put(INFINITY, neighbors.get(INFINITY) + 1);
		}
	}

	// --------------------------------------------------
	private void sendUpdate(RouterPacket pkt) {
		sim.toLayer2(pkt);

	}

	// --------------------------------------------------
	public void printDistanceTable() {
		myGUI.println("Current table for " + myID + "  at time " + sim.getClocktime());
		myGUI.println("Destino               " + "  Siguiente nodo ");
		for(Entry<Integer, Integer> i: fordwarding.entrySet()) {
			myGUI.println(i.getKey() + "                           " + i.getValue());
		}
		myGUI.println("");
		myGUI.println("Topología aprendida por el nodo " + myID + ":");
		for(Entry<Integer, HashMap<Integer, Integer>> i: matriz.entrySet()) {
			myGUI.println("  Vecinos del nodo " + i.getKey() + ": (nodo: costo)");
			for(Entry<Integer, Integer> j: i.getValue().entrySet()) {
				if (j.getKey() != INFINITY)
					myGUI.println("    " + j.getKey() + ": " + j.getValue());
			}
			myGUI.println("");
		}
		myGUI.println("  Vecinos del nodo " + myID + ": (nodo: costo)");
		for(Entry<Integer, Integer> j : neighbors.entrySet()) {
			if (j.getKey() != INFINITY)
				myGUI.println("    " + j.getKey() + ": " + j.getValue());
		}
		myGUI.println("____________________________________");
	}

	// --------------------------------------------------
	public void updateLinkCost(int dest, int newcost) {
		//matriz.put(pkt.sourceid, pkt.mincost);			
		if(newcost == INFINITY) {
			neighbors.remove(dest);

		}else {
			neighbors.put(dest, newcost);
		}
		for (Integer i : neighbors.keySet()) {
			if (i != INFINITY) {
				RouterPacket pkt3 = new RouterPacket(myID, i, neighbors);
				sendUpdate(pkt3);
			}
		}
		neighbors.put(INFINITY, neighbors.get(INFINITY)+1);
		

		dijktra();
	}
}
