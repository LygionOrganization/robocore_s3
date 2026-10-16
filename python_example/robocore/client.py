"""Send one factory-firmware command over HTTP, WebSocket or USB CDC.

pip install requests websocket-client pyserial
python client.py http http://192.168.4.1 '{"T":700,"first":0,"last":20}'
python client.py ws ws://192.168.4.1/ws '{"T":732,"id":1}'
python client.py usb COM15 '{"T":740}'
"""
import argparse
import json
import secrets
import time

def matches(raw, rid):
    try:
        result = json.loads(raw)
    except (ValueError, UnicodeDecodeError):
        return None
    return result if isinstance(result, dict) and result.get('rid') == rid else None

def request(transport, address, command, timeout=20):
    command = dict(command)
    if not isinstance(command.get('T'), int) or isinstance(command.get('T'), bool):
        raise ValueError('T must be an integer')
    command['rid'] = secrets.randbelow(2**32)
    payload = json.dumps(command, ensure_ascii=False, separators=(',', ':'))
    if len(payload.encode('utf-8')) > 255:
        raise ValueError('Command exceeds the 255-byte UTF-8 limit')
    rid = command['rid']
    stop = command['T'] == 0
    deadline = time.monotonic() + timeout
    if transport == 'http':
        import requests
        base = address.rstrip('/')
        response = requests.post(base + '/api/cmd', data=payload.encode(), headers={'Content-Type':'application/json'}, timeout=5)
        response.raise_for_status()
        if stop: return {'accepted':True, 'note':'stop has no execution reply'}
        while time.monotonic() < deadline:
            response = requests.get(base + '/api/result', timeout=3)
            response.raise_for_status()
            result = matches(response.text, rid)
            if result is not None: return result
            time.sleep(0.15)
    elif transport == 'ws':
        import websocket
        connection = websocket.create_connection(address, timeout=timeout)
        try:
            connection.send(payload)
            if stop: return {'sent':True, 'note':'stop has no execution reply'}
            while time.monotonic() < deadline:
                connection.settimeout(max(0.1, deadline-time.monotonic()))
                result = matches(connection.recv(), rid)
                if result is not None: return result
        finally: connection.close()
    elif transport == 'usb':
        import serial
        with serial.Serial(address, 1000000, timeout=0.2, write_timeout=3) as port:
            port.write(payload.encode('utf-8') + b'\n')
            if stop: return {'sent':True, 'note':'stop has no execution reply'}
            while time.monotonic() < deadline:
                # Bound host memory if a device emits a malformed line.
                result = matches(port.read_until(b'\n', 4096), rid)
                if result is not None: return result
    else: raise ValueError('Unknown transport')
    raise TimeoutError('No matching result. The command may have been overwritten; read device state before retrying writes.')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('transport', choices=['http','ws','usb'])
    parser.add_argument('address')
    parser.add_argument('command', help='JSON object, enclosed in shell quotes')
    args = parser.parse_args()
    result = request(args.transport, args.address, json.loads(args.command))
    print(json.dumps(result, ensure_ascii=False, indent=2))
