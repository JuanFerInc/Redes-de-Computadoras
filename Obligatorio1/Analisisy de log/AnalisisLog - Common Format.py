# -*- coding: utf-8 -*-
import re
import io
import geoip2.database
import operator
import time
import matplotlib.pyplot as plt



dicPaises = {} # key: codigo pais, value: (cantidad apariciones, bytes transmitidos)
dicHoras = {} # key: hora, value: (cantidad apariciones, bytes transmitidos)
dicIpCant = {} # key: ip, value: cantidad apariciones
dicHTTP = {} # key: version http, value: cantidad apariciones
dicIpAsnPais = {} # key: ip, value: (asn, codigo pais)
dicProyecto = {} # key: proyecto, value: (cantidad apariciones, bytes trasmitidos)
dicRespuesta = {} # key: codigo de respuesta, value: cantidad apariciones
totalByteIPv4 = 0
totalByteIPv6 = 0

setASN = set()
ipNoEncontradas = set()
setProyectoParciales = set()

reader1 = geoip2.database.Reader(r'GeoLite2-ASN_20190813\GeoLite2-ASN.mmdb')
reader2 = geoip2.database.Reader(r'GeoLite2-Country_20190813\GeoLite2-Country.mmdb')


def procesarIp(ip, bytes):
    global dicIpCant
    global IpNoEncontradas
    global dicPaises
    global setASN
    global dicIpAsnPais
    
    if ip not in dicIpCant.keys():
        try:
            response1 = reader1.asn(ip)
            response2 = reader2.country(ip)
            
            setASN.add(response1.autonomous_system_number)
            if response2.country.iso_code in dicPaises.keys():
                dicPaises[response2.country.iso_code] = (1 + dicPaises[response2.country.iso_code][0], bytes + dicPaises[response2.country.iso_code][1])
            else:
                dicPaises[response2.country.iso_code] = (1, bytes)
            dicIpAsnPais[ip] = (response1.autonomous_system_number,response2.country.iso_code)
        except:
            ipNoEncontradas.add(ip)
            if None in dicPaises.keys():
                dicPaises[None] = (1 + dicPaises[None][0], bytes + dicPaises[None][1])
            else:
                dicPaises[None] = (1, bytes)
        dicIpCant[ip] = 1
    else:
        dicIpCant[ip] += 1
        if ip not in ipNoEncontradas:
            if dicIpAsnPais[ip][1] in dicPaises.keys():
                dicPaises[dicIpAsnPais[ip][1]] = (1 + dicPaises[dicIpAsnPais[ip][1]][0], bytes + dicPaises[dicIpAsnPais[ip][1]][1])
            else:
                dicPaises[dicIpAsnPais[ip][1]] = (1, bytes)
        else:
            if None in dicPaises.keys():
                dicPaises[None] = (1 + dicPaises[None][0], bytes + dicPaises[None][1])
            else:
                dicPaises[None] = (1, bytes)



def precesarHTTP(version):
    global dicHTTP
    
    if version in dicHTTP.keys():
        dicHTTP[version] += 1
    else:
        dicHTTP[version] = 1


def procesarHora(fecha, bytes):
    global dicHoras
    
    horaRes = re.search(r':(\d\d):',fecha)
    hora = horaRes.group(1)

    if hora in dicHoras.keys():
        dicHoras[hora] = (1 + dicHoras[hora][0], bytes + dicHoras[hora][1])
    else:
        dicHoras[hora] = (1, bytes)

def procesarProyecto(proyecto, bytes):
    global dicProyecto

    if proyecto in dicProyecto.keys():
        dicProyecto[proyecto] = (1 + dicProyecto[proyecto][0], bytes+dicProyecto[proyecto][1])
    else:
        dicProyecto[proyecto] = (1,bytes)

def procesarRespuesta(codigo):
    global dicRespuesta
    
    if codigo in dicRespuesta.keys():
        dicRespuesta[codigo] += 1
    else:
        dicRespuesta[codigo] = 1



def esIpv4(ip):
    found = re.search(r':', ip)
    return found == None

def maxConexionHora():
    maxValor = 0
    hora = '00'
    for data in dicHoras:
       if(dicHoras[data][0] > maxValor):
           maxValor = dicHoras[data][0]
           hora = str(data)
    return hora,maxValor

def maxByteHora():
    maxValorByte = 0
    horaByte = '00'
    for data in dicHoras:
       if(dicHoras[data][1] > maxValorByte):

           maxValorByte = dicHoras[data][1]
           horaByte = str(data)
    return horaByte,maxValorByte

def proyectosParciales(proyecto,codigo):
    global setProyectoParciales

    if codigo == '206':
        setProyectoParciales.add(proyecto)
def barPlot():


   # global dicPaises

    dicPaises['None'] = dicPaises[None]
    del dicPaises[None]
    nuevodicPaises = {}

    for key,value in dicPaises.items():
        nuevodicPaises[key] = value[0]

    nuevodicPaises = dict(sorted(nuevodicPaises.items(), key=operator.itemgetter(1), reverse=True)[:20])
    fig = plt.figure()
    plt.bar(list(nuevodicPaises.keys()),nuevodicPaises.values(), color='g')
    fig.suptitle('Frecuencia de los paises por dia', fontsize = 20)
    plt.xlabel('Paises')
    plt.ylabel('Frecuencia por dia')
    fig.savefig('FrecuanciaPaises.png')


    fig1 = plt.figure()
    plt.bar(list(dicRespuesta.keys()), dicRespuesta.values(), color='g')
    fig1.suptitle('Frecuencia de codigo de respuesta por dia', fontsize = 20)
    plt.xlabel('Codigo Respuesta')
    plt.ylabel('Frecuencia por dia')


    fig1.autofmt_xdate()
    fig1.savefig('FrecuanciaRespuesta.png')

    #global dicProyecto


    dicProyectoTransmitido = {}
    for key,value in dicProyecto.items():
        dicProyectoTransmitido[key] = value[1]
    dicProyectoTransmitido = dict(sorted(dicProyectoTransmitido.items(), key=operator.itemgetter(1), reverse=True)[:10])


    fig2 = plt.figure()
    plt.bar(list(dicProyectoTransmitido.keys()),dicProyectoTransmitido.values(), color='g')
    fig2.suptitle('Cantidad de bytes servido por proyecto', fontsize=20)
    plt.xlabel('Proyectos')
    plt.ylabel('bytes')


    fig2.autofmt_xdate()
    fig2.savefig('bytesdeproyectos.png')



    fig3 = plt.figure()
    plt.bar(['IPv4','IPv6'],[totalByteIPv4,totalByteIPv6], color='g')
    fig3.suptitle('Bytes por protocolo', fontsize=20)
    plt.xlabel('Protocolo')
    plt.ylabel('bytes')
    fig3.savefig('bytesprotocolo.png')


    fig4 = plt.figure()
    plt.bar(list(dicHTTP.keys()), dicHTTP.values(), color='g')
    fig4.suptitle('Frecuencia de versiones de HTTP', fontsize=20)
    plt.xlabel('Versiones')
    plt.ylabel('Frecuencia')
    fig4.savefig('frecuenciaHTTP.png')

    fig5 = plt.figure()
    plt.bar(list(dicHoras.keys()),  [i[0] for i in dicHoras.values()], color='g')
    fig5.suptitle('Conexiones por hora', fontsize=20)
    plt.xlabel('Hora')
    plt.ylabel('Conexiones')
    fig5.savefig('conexionesHora.png')


    fig6 = plt.figure()
    plt.bar(list(dicHoras.keys()),  [i[1] for i in dicHoras.values()], color='g')
    fig6.suptitle('Bytes por hora', fontsize=20)
    plt.xlabel('Hora')
    plt.ylabel('Bytes')
    fig6.savefig('byteHora.png')

    plt.show()
def programa():

    start_time = time.time()
    totalByte = 0
    global totalByteIPv4
    global totalByteIPv6

    f = io.open('0001', 'r', newline='\n', encoding='utf-8')

    for line in f:

        # Grupos en order IP, ID de maquina, ID Usario, Fecha, Direccion client request, Version HTTP, Status Codigo, size
        # femSing = re.findall(r'(\S+) (\w+|-) (\w+|-) \[(.*?)\] "\w+ (?:/(\S*)/\S*|\*) HTTP/(\S+)" (\d+) (\d+|-)', line, flags=re.I)
        femSing = re.findall(r'(\S+) (\w+|-) (\w+|-) \[(.*?)\] "GET /(\S*?)/\S* HTTP/(\S+)" (\d+) (\d+|-)', line, flags=re.I)
        
        if femSing != []:
        
            ip = femSing[0][0]
            fecha = femSing[0][3]
            proyecto = femSing[0][4]
            versionHTTP = femSing[0][5]
            codigo = femSing[0][6]
            size = femSing[0][7]
            
            if size == '-':
                bytes = 0
            else:
                bytes = int(size)
                
            totalByte += bytes
            
            if esIpv4(ip):
                totalByteIPv4 += bytes
            else:
                totalByteIPv6 += bytes
            
            procesarIp(ip, bytes)
            
            precesarHTTP(versionHTTP)
            
            procesarHora(fecha, bytes)
            
            procesarProyecto(proyecto, bytes)
            
            procesarRespuesta(codigo)

            proyectosParciales(proyecto, codigo)
            
        else:
            femSing = re.findall(r'(\S+) (\w+|-) (\w+|-) \[(.*?)\] "\S+ \S+ HTTP/(\S+)" (\d+) (\d+|-)', line, flags=re.I)
            #
            # Falta considerar el caso que no es GET
            #



    f.close()

    promedioMbps = ((totalByte /(24*60*60))*8)/1048576

    hora,maxValor = maxConexionHora()
    horaByte,maxValorByte = maxByteHora()
    
    masActivas = dict(sorted(dicIpCant.items(), key=operator.itemgetter(1), reverse=True)[:20])

    res = "Tamano total: "+ str(totalByte) + "\n" +"Cantidad de IP's: " + str(len(dicIpCant)) + '\n' + "Cantidad de bytes a Ipv4: " + str(totalByteIPv4) + '\n' \
          + "Cantidad de bytes a Ipv6: " + str(totalByteIPv6) + '\n' + "Cantidad de Sistemas Autonomos: " + str(len(setASN)) + '\n\n' +"Paises y Frecuencia: Pais:(cantidad apariciones, bytes transmitidos)" + "\n"+ str(dicPaises) + "\n\n" + \
          "Cantidad de Paises Diferentes: "+ str(len(dicPaises)) + '\n\n' + "Cantidad de conecciones por hora: Hora:(cantidad apariciones, bytes transmitidos) "+ '\n' + str(dicHoras) + '\n\n' + "Hora con mayor trafico: " + str(hora) + "\nCantidad de trafico: " + str(maxValor) \
          +'\n\n'+ "Hora con mayor transmicion de Datos: "+ str(horaByte) + "\nCantidad Bytes Transmitido: "+ str(maxValorByte) + '\n\n' + "Versiones HTTP y cantidad" + '\n'+str(dicHTTP) + "\n\n" + 'Proyectos: Nombre proyecto:(cantidad apariciones, bytes trasmitidos) '+ '\n' + str(dicProyecto) + "\n\n" \
          + 'Codigos de respuesta: ' + str(dicRespuesta) + '\n\n' + '20 IP mas activas y frecuencia: IP: Frecuenca por dia ' +'\n' + str(masActivas) + '\n\n' + "Proyectos con descargas parciales:"+'\n' + str(setProyectoParciales)+ "\n\n" + "Mbps promedio: " + str(promedioMbps)

    barPlot()
    print("---TIME: %s SECONDS ---" % (time.time() - start_time))


    return res


if __name__ == '__main__':


    salida = programa()
    f = io.open('Res.txt', 'w', newline='\n', encoding='utf-8')
    for x in range(0, (len(salida))):
        f.write(salida[x])

    f.close()
