/**************************************************************************************************
  Filename:       ZigBeeNotification.java
  Revised:        $$
  Revision:       $$

  Description:    ZigBee Notification Class

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
import java.util.List;

import com.lightingcontroller.R;
import com.lightingcontroller.zllMain;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.os.Handler;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.TextView;

public class ZigbeeNotification {

	private static Activity acty = null;
	private static Dialog notificationDialog = null;
	private static boolean deviceRemove = false;

	static ArrayList<ZigbeeDevice> newDevNotifications = new ArrayList<ZigbeeDevice>();

	static int timeout = 5000;
	static int cnt = 0;
	
	public static void init(Activity a) {
		acty = a;
	}

	public static void closing() {
		closeDialog();
		acty = null;
	}

	public static void showNewDeviceNotification(final ZigbeeDevice device) {
		cnt++;
		if(cnt > 1)
			cnt = 0;
		
		if (acty != null) {
			if(notificationDialog != null)
			{
				newDevNotifications.add(device);
			}
			else
			{
				showNewDeviceNotificationDialog(device);
			}
		}
		else
		{
			
		}
		
		while (!newDevNotifications.isEmpty()) {
			if (acty != null) {
				if(notificationDialog == null)
				{
					ZigbeeDevice notifyDevice = newDevNotifications.remove(0);
					showNewDeviceNotificationDialog(notifyDevice);
				}
			}
		}
				
/*		
		// restart activity to update device drop down		
		Intent intent = new Intent();
		intent.setClass(acty, acty.getClass());
		acty.finish();
		acty.startActivity(intent);
*/
	}

	public static void showNewDeviceNotificationDialog(
			final ZigbeeDevice device) {
		// close any existing notification
		closeDialog();
		
		//put device into identify
		ZigbeeAssistant.setDeviceState(device,true);
		ZigbeeAssistant.setDeviceLevel(device,0xFF);
		ZigbeeAssistant.IdentifyDevice(device,(short) 600);
		
    	String Title = "New Device Found";
    	String Msg = "Please Enter the name of the device";
     	final EditText t = new EditText(acty);
     	t.setText(device.Name);

		final AlertDialog.Builder builder = new AlertDialog.Builder(acty);
		builder.setTitle(Title)
		.setMessage(Msg)
		.setView(t)
		.setPositiveButton("OK",  
		new DialogInterface.OnClickListener()
		{
			public void onClick(DialogInterface dialoginterface,int i){	
				ZigbeeAssistant.setDeviceName(device.Name, t.getText().toString());	
				zllMain.setCurrentDevice(device);
				closeDialog();
				//stop device identifying
				ZigbeeAssistant.IdentifyDevice(device,(short) 0);
			}
	    });
		

		try {
			acty.runOnUiThread(new Runnable() {
				public void run() {
					if (acty != null) {
						// if(acty.hasWindowFocus())
						// {
						notificationDialog = builder.create();
						notificationDialog.show();
						// }
					}
				}
			});
		} catch (Exception e) {

		}
	}

	public static void showRemoveDeviceNotification(String deviceStr,
			final int timeout) {
		if (acty != null) {
			// close any existing notification
			closeDialog();

			final AlertDialog.Builder builder = new AlertDialog.Builder(acty);
			builder.setTitle("ZigBee Notification")
					.setCancelable(false)
					.setMessage("Device Removed: " + deviceStr)
					.setPositiveButton("Ok",
							new DialogInterface.OnClickListener() {
								public void onClick(DialogInterface dialog,
										int id) {
									// restart activity to update device drop
									// down
									Intent intent = new Intent();
									intent.setClass(acty, acty.getClass());
									acty.finish();
									acty.startActivity(intent);
									closeDialog();
								}
							});

			try {
				acty.runOnUiThread(new Runnable() {
					public void run() {
						if (acty != null) {
							// if(acty.hasWindowFocus())
							// {
							notificationDialog = builder.create();
							notificationDialog.show();
							// }
						}
					}
				});
			} catch (Exception e) {

			}

			new Thread(new Runnable() {
				public void run() {
					try {
						Thread.sleep(timeout);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					closeDialog();
				}
			}).start();

		}
	}

	public static void showGeneralNotification(String notification,
			final int timeout) {
		if (acty != null) {
			// close any existing notification
			closeDialog();

			final AlertDialog.Builder builder = new AlertDialog.Builder(acty);
			builder.setTitle("ZigBee Notification").setCancelable(false)
					.setMessage(notification);

			try {
				acty.runOnUiThread(new Runnable() {
					public void run() {
						if (acty != null) {
							// if(acty.hasWindowFocus())
							// {
							notificationDialog = builder.create();
							notificationDialog.show();
							// }
						}
					}
				});
			} catch (Exception e) {

			}

			new Thread(new Runnable() {
				public void run() {
					try {
						Thread.sleep(timeout);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					closeDialog();
				}
			}).start();
		}
	}

	private static void closeDialog() {
		if (notificationDialog != null) {
			notificationDialog.cancel();
		}
		notificationDialog = null;
	}
}
