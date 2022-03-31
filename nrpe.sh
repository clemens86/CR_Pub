echo "installing nrpe"
sudo apt-get install nagios-nrpe-server -y
echo "."
sleep 0.1
echo "."
sleep 0.1
echo "Descending into /usr/lib/nagios/plugins"
cd /usr/lib/nagios/plugins
echo "."
echo "Downloading check_mem"
sudo wget https://raw.githubusercontent.com/justintime/nagios-plugins/master/check_mem/check_mem.pl
echo "moving it"
sudo mv check_mem.pl check_mem
echo "chmod +x"
sudo chmod +x check_mem
echo "."
sleep 0.1
echo "Checking Output"
sudo ./check_mem -f -w 20 -c 10
echo "."
sleep 0.1
echo "Downloading check_ro_mounts"
sudo wget https://raw.githubusercontent.com/clemens86/CR_Pub/main/check_ro_mounts
echo "chmod +x"
sudo chmod +x check_ro_mounts
echo "."
sleep 0.1
echo "Checking Output"
./check_ro_mounts
sleep 1
echo "."
sleep 0.1
echo ".."
sleep 0.1
echo "..."
echo "retreating out of dir..."
sleep 0.1
cd ~/
echo "."
sleep 0.1
echo "Adding allowed hosts to /etc/nagios/nrpe_local.cfg"
echo 'allowed_hosts=10.0.0.0/24, 10.0.1.0/24, 10.0.2.0/24, 10.0.3.0/24, 10.1.0.0/24, 10.1.1.0/24, 10.2.0.0/24, 10.2.1.0/24, 10.2.2.0/24, 10.2.3.0/24, 10.2.4.0/24' | sudo tee -a /etc/nagios/nrpe_local.cfg
echo "."
sleep 0.1
echo "Changing System Load Settings in /etc/nagios/nrpe_local.cfg"
echo 'command[check_load]=/usr/lib/nagios/plugins/check_load -r -w .25,.20,.15 -c .35,.30,.25' | sudo tee -a /etc/nagios/nrpe_local.cfg
echo "Adding Swap Check in /etc/nagios/nrpe_local.cfg"
echo 'command[check_swap]=/usr/lib/nagios/plugins/check_swap -w 5 -c 3' | sudo tee -a /etc/nagios/nrpe_local.cfg
echo "Adding Mem Check in /etc/nagios/nrpe_local.cfg"
echo 'command[check_mem]=/usr/lib/nagios/plugins/check_mem -f -w 4 -c 2' | sudo tee -a /etc/nagios/nrpe_local.cfg
echo "Adding check ro mounts in /etc/nagios/nrpe_local.cfg"
echo 'command[check_ro_mounts]=/usr/lib/nagios/plugins/check_ro_mounts -x /sys/fs/cgroup' | sudo tee -a /etc/nagios/nrpe_local.cfg
echo "."
sleep 0.1
echo "."
sleep 0.1
echo "restarting NRPE Server"
sudo /etc/init.d/nagios-nrpe-server restart
