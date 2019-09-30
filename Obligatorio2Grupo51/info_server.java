package Server;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.PatternSyntaxException;

public class info_server {
	private static Map<String,String> Usuarios = new HashMap<String,String>();
	
	private static void cargarDatos() {
		//Cargar server 1
		Usuarios.put("matias", "192.168.56.102");
		Usuarios.put("martin", "192.168.56.102");
		Usuarios.put("eduardo", "192.168.56.102");
		Usuarios.put("arierl", "192.168.56.102");
		
		//cargar server 2
		Usuarios.put("lucia", "192.168.56.103");
		Usuarios.put("sara", "192.168.56.103");
		Usuarios.put("clara", "192.168.56.103");
		Usuarios.put("marta", "192.168.56.103");
		
	}
	
	public static void main(String[] args) {
		cargarDatos();
		
		int puerto = 9876;
		if (args.length > 0) {
			puerto = new Integer(args[0]);
		}
		try {
			//Pido socket para el servidor
			DatagramSocket socketServidor = 
				new DatagramSocket(puerto);
			int i = 0;
			while (true) {
				
				String PEDIDO = "";
				
				
				
				
				
				//Pide memoria para recibir el paquete
				DatagramPacket paqueteRecepcion = 
					new DatagramPacket(new byte[1024], 1024);
				//recibe paquete
				socketServidor.receive(paqueteRecepcion);
				
				//Convierte paquete en string
				String frase = 
					new String(paqueteRecepcion.getData(), 0, 
							paqueteRecepcion.getLength());
				//Obtiene IP y puerto del que mando
				InetAddress direccionDestino = paqueteRecepcion.getAddress();
				int puertoDestino = paqueteRecepcion.getPort();
				
				//Convierte todo en mayuscula, no es necesario
				//byte[] data = frase.toUpperCase().getBytes();
				String[] split ;
				String Codigo ;
				String nombre;
				
				//partir el mensaje en todas sus partes
				try {
					split = frase.split(" ");
					Codigo = split[0];
					nombre = split[1];
					PEDIDO = "Pedido: "+ i + " " + Codigo +" " + nombre ;
				}catch(ArrayIndexOutOfBoundsException exception) {
					Codigo = "FORMATO";
					nombre = "";
					PEDIDO = "Pedido: "+ i + " " + Codigo +" " + nombre ;
				}
				
			
				//buscar nombre del usuario
				String res = Usuarios.get(nombre);
				
				//si no se encuentra manda Codigo + error o IP de servidor 
				if(res == null) {
					res = Codigo + " ERROR";
					PEDIDO  = PEDIDO + "Respuesta: " + res;
					
					
				}else {
					res = Codigo +" " + res;
					PEDIDO  = PEDIDO + " Respuesta: " + res;
				}
				
				//Convierte en bytes
				byte[] data = res.getBytes();
				
				
				DatagramPacket paqueteEnvio = 
					new DatagramPacket(
							data, res.length(), 
							direccionDestino, puertoDestino);
	
				socketServidor.send(paqueteEnvio);
				System.out.println(PEDIDO);
				i = i+1;
			}
			
			
			
		}catch(IOException exception) {
			System.out.println(exception);
			
		}
	}
}


