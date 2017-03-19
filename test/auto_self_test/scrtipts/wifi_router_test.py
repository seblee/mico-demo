import wifi
import router
import string
import random

wifi = wifi.WiFi()
router = router.Router()

def randstr(length):
	letters = string.ascii_letters+string.digits
	return ''.join([random.choice(letters) for i in range(length)])

router_config = ({'ssid':randstr( 1), 'key':None, 'encryption':'none', 'channel': 1},
{'ssid':randstr( 1), 'key':randstr( 8), 'encryption': 'psk', 'channel': 6},
{'ssid':randstr(16), 'key':randstr(32), 'encryption':'psk2', 'channel': 11},
{'ssid':randstr(32), 'key':randstr(63), 'encryption':'psk2', 'channel': 13})

for config in router_config:
	ssid, key, encryption, channel = config['ssid'], config['key'], config['encryption'], config['channel']
	print('SSID = %s, key = %s, encryption = %s, channel = %d'%(ssid, key, encryption, channel))
	router.ssid = ssid
	router.encryption = encryption
	if encryption != 'none':
		router.key = key
	router.channel = channel
	router.commit()
	for i in range(10):
		print('Connecting to router...')
		ip, gw = wifi.connect(ssid, key)
		if(ip and gw):
			print('Connected')
			wifi.disconnect()
		else:
			print('Connect failed')
	sleep(2)
