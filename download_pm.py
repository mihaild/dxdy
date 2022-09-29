import requests
import sys

cookies = dict(tuple(s.split('=')) for s in sys.argv[1].split('; '))
START = 0
END = 10000

headers = {
    'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9',
    'Accept-Language': 'en-US,en;q=0.9,ru-RU;q=0.8,ru;q=0.7',
    'Cache-Control': 'max-age=0',
    'Connection': 'keep-alive',
    'Origin': 'https://dxdy.ru',
    'Referer': 'https://dxdy.ru/ucp.php?i=pm&mode=view&action=view_folder&f=0&start=50',
    'Sec-Fetch-Dest': 'document',
    'Sec-Fetch-Mode': 'navigate',
    'Sec-Fetch-Site': 'same-origin',
    'Sec-Fetch-User': '?1',
    'Upgrade-Insecure-Requests': '1',
    'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36',
    'sec-ch-ua-mobile': '?0',
    'sec-ch-ua-platform': '"Linux"',
}

params = {
    'i': 'pm',
    'mode': 'view',
    'action': 'view_folder',
    'f': '0',
    'start': '50',
}

data = {
    'delimiter': ',',
    'enclosure': '"',
    'export_option': 'CSV',
    'submit_export': 'Этот список',
}

for i in range(START, END, 50):
    print("Running ", i)
    params['start'] = i
    response = requests.post('https://dxdy.ru/ucp.php', params=params, cookies=cookies, headers=headers, data=data)
    try:
        text = response.text.encode('ISO-8859-1').decode('UTF-8')
        f = open(str(i) + '.csv', 'w')
        f.write(text)
        f.close()
    except:
        print(response)
        print(response.text)
        raise
