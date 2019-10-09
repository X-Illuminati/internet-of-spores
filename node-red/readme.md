# Node-Red flows.json
The node-red flows.json implements the measurement data collection and firmware
update procedure.

## Dependencies
There are some dependencies on community-contributed node programs.
These are available through the node-red palette manager and more details can
be found on nodered.org.
The version numbers are the versions I have tested. Presumably newer versions
will also work.

* [node-red-contrib-readdir v1.0.1](https://flows.nodered.org/node/node-red-contrib-readdir)
* [node-red-contrib-md5 v1.0.4](https://flows.nodered.org/node/node-red-contrib-md5)
* [node-red-contrib-influxdb v0.3.1](https://flows.nodered.org/node/node-red-contrib-influxdb)

# Node-Red systemd unit file
The node-red.service is a systemd unit file that can be used to create a
systemd service to start and run the flows. It is compatible with being run as
a systemd user-service if installed in ~/.local/share/systemd/user/.

```
systemctl --user start node-red.service
systemctl --user restart node-red.service
systemctl --user stop node-red.service
systemctl --user status node-red.service
```
