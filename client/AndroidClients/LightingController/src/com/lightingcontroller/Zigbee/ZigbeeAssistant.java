/**************************************************************************************************
  Filename:       ZigBeeAssistant.java
  Revised:        $$
  Revision:       $$

  Description:    ZigBee Assistant

    Copyright (C) {2012} Texas Instruments Incorporated - http://www.ti.com/


   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

     Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the
     distribution.

     Neither the name of Texas Instruments Incorporated nor the names of
     its contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
**************************************************************************************************/

package com.lightingcontroller.Zigbee;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.HashMap;
import java.util.List;

import com.lightingcontroller.Zigbee.ZigbeeScene;

import android.app.Activity;
import android.graphics.Color;
import android.util.Log;
import android.widget.Toast;

public class ZigbeeAssistant implements Runnable {

	//public static final boolean USE_JNI = true;
	public static final boolean USE_JNI = false;

	public static boolean gateWayConnected = false;
	public static boolean initializing = true;
	
	private static String TAG = "ZigbeeAssistant";
	public static ZigbeeSrpcClient zigbeeSrpcClient;
	public static String ZigbeeSrpcClientGatewayIp = "192.168.1.220";
	
	static Thread thread;
	static Activity mainAct;
	
	static List<ZigbeeDevice> ourDevices = new ArrayList<ZigbeeDevice>();
	static List<ZigbeeGroup> ourGroups =  new ArrayList<ZigbeeGroup>();
	static List<ZigbeeScene> ourScenes =  new ArrayList<ZigbeeScene>();
	
	private static int LightDeviceIdx = 0;

	static public int lastTempReading = 65535;
	static public List<Integer> powerReadings = new ArrayList<Integer>();
	static public List<Long> powerReadingsTimes = new ArrayList<Long>();
	static public HashMap<Short,Long> nwrkToIEEE = new HashMap<Short,Long>();
		
	public static native int runMain();
	public static native boolean isRunning();
	public static native short requestThermostatReading(short networkaddr, char end); // Get ThermostatReading
	public static native short requestPowerReading(short networkaddr, char end); // Get ThermostatReading
	public static native void setDeviceState(short networkaddr, char end, boolean state);	// Set device state on/off
	public static native void setDeviceLevel(short networkaddr, char end, char level, short transitionTime);	// Set device level/dim
	public static native void setDeviceColour(short networkaddr, char end, char hue, char saturation, short transitionTime);	// Set device Color	
	
	public static native void bindDevices(short network_a, char end_a, int ieel_a, int ieeh_a,  
									  	  short network_b, char end_b, int ieel_b, int ieeh_b,
									  	  short clusterId);	// Bind Devices
	
	
	
	static
	{
		if(USE_JNI)			
		   System.loadLibrary("zigbee_assistant_jni");  		//Loading native library	
		
	}	
	
	public static void notifyUser(String s)
	{
		ZigbeeNotification.showNotification(s);
	}

	public static List<ZigbeeDevice> getDevices()
	{
		return ourDevices;
	}	
	
	public static void addGroup(ZigbeeDevice d, String groupName )
	{		
		//ZigbeeGroup newGroup = new ZigbeeGroup(groupName);
		//ourGroups.add(newGroup);		
		
		ZigbeeSrpcClient.addGroup((short) d.NetworkAddr,d.EndPoint, groupName);
	}
	
	public static void storeScene(String sceneName, int groupId )
	{						
		ZigbeeSrpcClient.storeScene( sceneName, groupId );
	}
	
	public static void recallScene(String sceneName, int groupId )
	{					
		ZigbeeSrpcClient.recallScene(sceneName, groupId );
	}

	public void descoverGroups()
	{
		ZigbeeSrpcClient.discoverGroups( );
	}
	
	public static List<ZigbeeGroup> getGroups()
	{
		return ourGroups;
	}		

	public static List<ZigbeeScene> getScenes()
	{
		return ourScenes;
	}
	
	public static void deviceAnnounce(int nwrk, long ieee1) 
	{
		// TODO Auto-generated method stub

		if (nwrkToIEEE.containsKey(nwrk))
			nwrkToIEEE.remove(nwrk);
		nwrkToIEEE.put((short)nwrk, ieee1);		
	}

	
	public static void deviceAnnounce(int nwrk, int ieee1, int ieee2)
	{
		long t2;
		long t = (((long)ieee2))<<32;
		
		if(ieee1 < 0)
		{
			long t3 = 2147483647;
			t3 = t3 + 2147483647;
			t3 = t3 + 2;
			t2 = (((long)ieee1));		
			t2 = t2+t3;
		}
		else
		{
			t2 = ieee1;			
		}
		t = t | t2;
		if (nwrkToIEEE.containsKey(nwrk))
			nwrkToIEEE.remove(nwrk);
		nwrkToIEEE.put((short)nwrk, t);
	}

	public static void newDevice(int ProfileId, int DeviceId, int NetworkAddr, char EndPoint, byte ieee[], String deviceName)
	{
		ZigbeeDevice oldD = ifDeviceExists(NetworkAddr,EndPoint);
		if (oldD != null)
		{
			if (oldD.ProfileId == ProfileId)
				return;
			oldD = new ZigbeeDevice(ProfileId, DeviceId, NetworkAddr, EndPoint, ieee, deviceName, LightDeviceIdx++);
			return;
		}
		ZigbeeDevice newDevice = new ZigbeeDevice(ProfileId, DeviceId, NetworkAddr, EndPoint,  ieee, deviceName, LightDeviceIdx++);
		ourDevices.add(newDevice);
	}

	public static void newGroup( String groupName )
	{		
		ZigbeeGroup newGroup = new ZigbeeGroup(groupName);
		ourGroups.add(newGroup);	
		
		//Create new group on gateway (with invalid nwkAddr so not device is added to the group)
		ZigbeeSrpcClient.addGroup((short) 0xFFFF, (char) 0xFF, groupName);
	}	
	
	public static void newGroup(String groupName, int groupId, int status)
	{
		ZigbeeGroup group=null;
        List<ZigbeeGroup> groupList = ZigbeeAssistant.getGroups();

        //find the group        
        for(int i=0; i < groupList.size(); i++)
        {
        	if(groupName.equals(groupList.get(i).getGroupName()))
        	{
        		group = groupList.get(i);
        		group.setGroupId(groupId);
        		group.setStatus(status);
        		break;
        	}
        }
        
        if(group == null)
        {		
        	group = new ZigbeeGroup(groupName, groupId, status);   
        	ourGroups.add(group);
        }
	}

	public static void newScene(String sceneName, int groupId, byte sceneId, int status)
	{
		ZigbeeScene scene=null;
        List<ZigbeeScene> sceneList = ZigbeeAssistant.getScenes();

        //find the scene        
        for(int i=0; i < sceneList.size(); i++)
        {
        	if(sceneName.equals(sceneList.get(i).getSceneName()))
        	{
        		scene = sceneList.get(i);
        		scene.setGroupId(groupId);
        		scene.setSceneId(sceneId);
        		scene.setStatus(status);
        		break;
        	}
        }
        
        if(scene == null)
        {		
        	scene = new ZigbeeScene(sceneName, groupId, sceneId, status);   
        	ourScenes.add(scene);
        }
	}
	
	public static void newScene(String sceneName, int groupId )
	{				
		ZigbeeScene newScene = new ZigbeeScene(sceneName, groupId);
		if(ourScenes.contains(newScene) == false)
		{
			ourScenes.add(newScene);
		}
		
		ZigbeeSrpcClient.storeScene( sceneName, groupId );
	}
	
	public static boolean bindDevicesOnOff(ZigbeeDevice a, ZigbeeDevice b)
	{
		return bindDevices(a,b,0x0006);
	}
	
	public static boolean bindDevicesAll(ZigbeeDevice a, ZigbeeDevice b)
	{		
		bindDevices(a,b,0x0006);
		bindDevices(a,b,0x0008);
		bindDevices(a,b,0x0300);
		bindDevices(a,b,0x0005);
		bindDevices(a,b,0x0004);
		
		return true;
	}	
	
	public static boolean bindDevices(ZigbeeDevice a, ZigbeeDevice b, int cluster)
	{			
		ZigbeeSrpcClient.bindDevices((short)a.NetworkAddr,a.EndPoint, a.Ieee,
				(short)b.NetworkAddr,b.EndPoint,b.Ieee,(short)cluster);
		
		return true;
	}

	public static void setDeviceName(String oldName, String newName)
	{
		int last = ourDevices.size()-1;
		int i = last;
		ZigbeeDevice d = null;
		for (i = last ; i >= 0 ; i--)
		{
			d = ourDevices.get(i);
			if (d.Name == oldName)
				break;
		}
		
		if( (d != null) && (newName != null) )
		{
			d.Name = newName;
			//Need to push this back to server
			ZigbeeSrpcClient.changeDeviceName(d, newName);
		}
	}
	
	public static ZigbeeDevice ifDeviceExists(int NetworkAddr, char EndPoint)
	{
		int last = ourDevices.size()-1;
		int i = last;
		ZigbeeDevice ret = null;
		for (i = last ; i >= 0 ; i--)
		{
			ret = ourDevices.get(i);
			if (ret.NetworkAddr == NetworkAddr && ret.EndPoint == EndPoint)
				break;
		}
		if (i < 0) 
			return null;
		else
			return ret;
	}
	
	public static boolean hasAnySwitchers()
	{
		for (int i = 0 ; i < ourDevices.size() ; i++)
		{
			ZigbeeDevice d = ourDevices.get(i);	
			if (d.hasOutSwitch)
				return true;
		}
		return false;	
	}
	
	public static void newDeviceCluster(int NetworkAddr, char EndPoint, boolean in, int ClusterId)
	{
		ZigbeeDevice toAddto = ifDeviceExists(NetworkAddr, EndPoint);
		if (toAddto == null) 
		{
			Log.w(TAG, "Couldn't find Device");
			return;
		}
		
		toAddto.addCluster(in, ClusterId);	
	}
	
	public ZigbeeAssistant()
	{
		ourDevices.clear();
		LightDeviceIdx = 0;
	}

	public static String getInfoString()
	{
		if (ourDevices.size() == 0)
			return "No devices detected.";
			
		String s = "";
		for (int i = 0 ; i < ourDevices.size() ; i++)
		{
			ZigbeeDevice d = ourDevices.get(i);
			s += d.Name + " - 0x" + Integer.toHexString(d.NetworkAddr) + "("+(int)d.EndPoint+")\n";
			if (d.hasSwitchable)
				s+="\t: Switchable\n";
			if (d.hasDimmable)
				s+="\t: Dimmable\n";
			if (d.hasColourable)
				s+="\t: Colourable\n";
			if (d.hasThermometer)
				s+="\t: Measures Temperature\n";
			if (d.hasPowerUsage)
				s+="\t: Measures Power Usage\n";			
			if (d.hasOutSwitch)
				s+="\t: Switches Others\n";
			if (d.hasOutLeveL)
				s+="\t: controls level of Others\n";	
			if (d.hasOutColor)
				s+="\t: controls color of Others\n";
			if (d.hasOutScene)
				s+="\t: controls scenes of Others\n";
			if (d.hasOutGroup)
				s+="\t: controls groups of Others\n";			
		}
		return s;
	}
	
	public static ArrayList<ZigbeeDevice> getSwitchers()
	{
		ArrayList<ZigbeeDevice> s = new ArrayList<ZigbeeDevice>();
		for (int i = 0 ; i < ourDevices.size() ; i++)
		{
			ZigbeeDevice d = ourDevices.get(i);	
			if (d.hasOutSwitch)
				s.add(d);
		}
		return s;
	}	

	public static ArrayList<ZigbeeDevice> getSwitchable()
	{
		ArrayList<ZigbeeDevice> s = new ArrayList<ZigbeeDevice>();
		for (int i = 0 ; i < ourDevices.size() ; i++)
		{
			ZigbeeDevice d = ourDevices.get(i);	
			if (d.hasColourable || d.hasSwitchable || d.hasDimmable)
				s.add(d);
		}
		return s;
	}	
	
	public static void haveTemperature(int NetworkAddr, char EndPoint, int data)
	{
		lastTempReading = data;
	}
	
	public static void setDeviceState(ZigbeeDevice d, boolean state)
	{
		if(USE_JNI)			
			setDeviceState((short) d.NetworkAddr,d.EndPoint,state);
		else
			ZigbeeSrpcClient.setDeviceState((short) d.NetworkAddr, ZigbeeSrpcClient.Addr16Bit, d.EndPoint,state);			
	}

	public static void setDeviceState(ZigbeeGroup g, boolean state)
	{
		ZigbeeSrpcClient.setDeviceState((short) g.getGroupId(), ZigbeeSrpcClient.AddrGroup, (char) 0xFF, state);				
	}
	
	public static void setDeviceColour(ZigbeeDevice d, int colour)
	{
		float[] hsv = new float[3];
		Color.colorToHSV(colour, hsv);
		if (USE_JNI)
			setDeviceColour((short) d.NetworkAddr,d.EndPoint,(char)((hsv[0]/360)*0xFF), (char)(hsv[1]*0xFF),(short)10);
		else
			ZigbeeSrpcClient.setDeviceColor((short) d.NetworkAddr, ZigbeeSrpcClient.Addr16Bit, d.EndPoint,(char)((hsv[0]/360)*0xFF), (char)(hsv[1]*0xFF),(short)10);
	}

	public static void setDeviceHueSat(ZigbeeDevice d, byte hue, byte sat)
	{
		if (USE_JNI)
			setDeviceColour((short) d.NetworkAddr,d.EndPoint,(char) hue, (char) sat,(short)10);
		else
			ZigbeeSrpcClient.setDeviceColor((short) d.NetworkAddr, ZigbeeSrpcClient.Addr16Bit, d.EndPoint, (char) hue, (char) sat,(short)10);
	}

	public static void setDeviceHueSat(ZigbeeGroup g, byte hue, byte sat)
	{
			ZigbeeSrpcClient.setDeviceColor((short) g.getGroupId(), ZigbeeSrpcClient.AddrGroup, (char) 0xFF, (char) hue, (char) sat,(short)10);
	}	
	
	public static void setDeviceLevel(ZigbeeDevice d, int colour)
	{
		float[] hsv = new float[3];
		Color.colorToHSV(colour, hsv);	
		setDeviceLevel(d,(char)((hsv[2])*0xFF));
	}
	
	public static void setDeviceLevel(ZigbeeDevice d, char level)
	{
		if(USE_JNI)	
			setDeviceLevel((short) d.NetworkAddr,d.EndPoint,level,(short)10);
		else
			ZigbeeSrpcClient.setDeviceLevel((short) d.NetworkAddr, ZigbeeSrpcClient.Addr16Bit, d.EndPoint,level,(short)10);
	}

	public static void setDeviceLevel(ZigbeeGroup g, char level)
	{
			ZigbeeSrpcClient.setDeviceLevel((short) g.getGroupId(), ZigbeeSrpcClient.AddrGroup, (char) 0xFF,level,(short)10);
	}	

	public static void getDeviceState(ZigbeeDevice d)
	{
		ZigbeeSrpcClient.getDeviceState((short) d.NetworkAddr, ZigbeeSrpcClient.Addr16Bit, d.EndPoint);			
	}
	
	public static void getDeviceLevel(ZigbeeDevice d)
	{
		ZigbeeSrpcClient.getDeviceLevel((short) d.NetworkAddr, ZigbeeSrpcClient.Addr16Bit, d.EndPoint);			
	}	

	public static void getDeviceHue(ZigbeeDevice d)
	{
		ZigbeeSrpcClient.getDeviceHue((short) d.NetworkAddr, ZigbeeSrpcClient.Addr16Bit, d.EndPoint);			
	}		

	public static void getDeviceSat(ZigbeeDevice d)
	{
		ZigbeeSrpcClient.getDeviceSat((short) d.NetworkAddr, ZigbeeSrpcClient.Addr16Bit, d.EndPoint);			
	}
	
	public void run() {
		if(USE_JNI)
		{
			Log.d(TAG, "Begin Zigbee");
			final int returnvalue = runMain();
			mainAct.runOnUiThread(new Runnable() {
				public void run() {
				Toast.makeText(mainAct, "Zigbee has failed ("+returnvalue+") \n - Is the dongle connected and chmodded?", Toast.LENGTH_LONG).show();
			}
			});
		}
		else
		{
			Log.d(TAG, "Begin Zigbee");		
			zigbeeSrpcClient = new ZigbeeSrpcClient();
		}			
	}
}
