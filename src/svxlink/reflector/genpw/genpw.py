#!/usr/bin/python3

from flask import Flask, escape, request, make_response, jsonify, render_template_string
import random
import string
import dbm
import time
import json
import os
import math

page_template = '''
<!doctype html>
<html>
<head>
  <title>SvxReflector Password Generator</title>
</head>
<body>
<h1>SvxReflector Password Generator</h1>
Copy the callsign and password configuration to your SvxLink ReflectorLogic
configuration. A new password is generated every time this page is reloaded so
when you have settled for a password, don't reload. Send the timestamp to the
SvxReflector administrator as a verification of that the correct password
generation is chosen.
<p/>
Timestamp: {{ data.ts }}
<p/>
<pre>
[ReflectorLogic]
...
CALLSIGN={{ data.callsign }}
AUTH_KEY="{{ data.password }}"
...
</pre>
<hr/>
</body>
</html>
'''

def random_pw_string(length):
    alphabet = string.ascii_letters + string.digits \
            + '!#$%&*+:;<=>?_'
    alphabet.translate({ord('"'): None, ord('\\'): None})
    result_str = ''.join((random.choice(alphabet) for i in range(length)))
    return result_str

app = Flask(__name__)

@app.route('/')
def genpw():
    callsign = escape(request.args.get("callsign"))
    if callsign == 'None' or len(callsign) == 0:
        response = make_response('BAD REQUEST\n', 400)
        response.mimetype = "text/plain"
    else:
        frac, epoch = math.modf(time.time())
        frac_ms = int(1000 * frac)
        ts =  '%s.%03uZ' % (time.strftime('%Y-%m-%dT%H:%M:%S', \
                time.gmtime(epoch)), frac_ms)
        pw = random_pw_string(30)
        data = {
                'remote_addr': request.remote_addr,
                'ts': ts,
                'password': pw
                }
        os.umask(0o007)
        with dbm.open('db/svxreflector-genpw', 'c', 0o660) as db:
            db[callsign] = json.dumps(data)
        data['callsign']=callsign
        #print(data)
        response = make_response(render_template_string(page_template, data=data), 200)
    response.headers['Cache-Control'] = 'no-store'
    return response

if __name__ == '__main__':
    app.run(debug=False)
