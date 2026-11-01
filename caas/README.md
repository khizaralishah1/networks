You open a browser, type in a domain name and Enter. The http request goes to a Public DNS server whose ip you already know. This server responds with an http message that contains the ip of the domain you typed in.

Your browser sends an http request (to bring main html page) to that ip.

For now, that ip was of an edge gateway. This gateway will accept this request and see the domain name. It will find the service associated with this domain name from its service registery. How it was added is another story.

Gateway finds a list of endpoints (machines) where this service, just a python app, is running. It routes our http request to a relevant endpoint - which is one of the many replicas of this service running in any of the machines in the data center. The way it forwards the request to that python app (service) is by making a temporary tcp socket and connecting to it internally. The python app (running in the data center) then receives this request, does something, and forwards the response to the gateway, which sends it back to us, the client.

When client sends a huge number of http requests to the edge gateway, it will crash. Next step is to fix this problem