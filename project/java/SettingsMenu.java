/*
Simple DirectMedia Layer
Java source code (C) 2009-2014 Sergii Pylypenko

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required. 
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

package net.sourceforge.clonekeenplus;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;
import android.util.Log;
import java.io.*;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Environment;
import android.os.StatFs;
import java.util.Locale;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.zip.GZIPInputStream;
import java.util.Collections;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import java.lang.String;
import android.graphics.Matrix;
import android.graphics.RectF;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.FrameLayout;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.BitmapFactory;
import android.graphics.Bitmap;
import android.widget.TextView;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.Button;
import android.view.View;
import android.widget.LinearLayout;
import android.text.Editable;
import android.text.SpannedString;
import android.content.Intent;
import android.app.PendingIntent;
import android.app.AlarmManager;
import android.util.DisplayMetrics;
import android.net.Uri;
import java.util.concurrent.Semaphore;
import java.util.Arrays;
import android.graphics.Color;
import android.hardware.SensorEventListener;
import android.hardware.SensorEvent;
import android.hardware.Sensor;
import android.widget.Toast;


class SettingsMenu
{
	public static abstract class Menu
	{
		// Should be overridden by children
		abstract void run(final MainActivity p);
		abstract String title(final MainActivity p);
		boolean enabled()
		{
			return true;
		}
		// Should not be overridden
		boolean enabledOrHidden()
		{
			for( Menu m: Globals.HiddenMenuOptions )
			{
				if( m.getClass().getName().equals( this.getClass().getName() ) )
					return false;
			}
			return enabled();
		}
		void showMenuOptionsList(final MainActivity p, final Menu[] list)
		{
			menuStack.add(this);
			ArrayList<CharSequence> items = new ArrayList<CharSequence> ();
			for( Menu m: list )
			{
				if(m.enabledOrHidden())
					items.add(m.title(p));
			}
			AlertDialog.Builder builder = new AlertDialog.Builder(p);
			builder.setTitle(title(p));
			builder.setItems(items.toArray(new CharSequence[0]), new DialogInterface.OnClickListener()
			{
				public void onClick(DialogInterface dialog, int item)
				{
					dialog.dismiss();
					int selected = 0;

					for( Menu m: list )
					{
						if(m.enabledOrHidden())
						{
							if( selected == item )
							{
								m.run(p);
								return;
							}
							selected ++;
						}
					}
				}
			});
			builder.setOnCancelListener(new DialogInterface.OnCancelListener()
			{
				public void onCancel(DialogInterface dialog)
				{
					goBackOuterMenu(p);
				}
			});
			AlertDialog alert = builder.create();
			alert.setOwnerActivity(p);
			alert.show();
		}
	}

	static ArrayList<Menu> menuStack = new ArrayList<Menu> ();

	public static void showConfig(final MainActivity p, final boolean firstStart)
	{
		Settings.settingsChanged = true;
		if( Globals.OptionalDataDownload == null )
		{
			String downloads[] = Globals.DataDownloadUrl;
			Globals.OptionalDataDownload = new boolean[downloads.length];
			boolean oldFormat = true;
			for( int i = 0; i < downloads.length; i++ )
			{
				if( downloads[i].indexOf("!") == 0 )
				{
					Globals.OptionalDataDownload[i] = true;
					oldFormat = false;
				}
			}
			if( oldFormat )
				Globals.OptionalDataDownload[0] = true;
		}

		if(!firstStart)
			new MainMenu().run(p);
		else
		{
			if( Globals.StartupMenuButtonTimeout > 0 ) // If we did not disable startup menu altogether
			{
				for( Menu m: Globals.FirstStartMenuOptions )
				{
					boolean hidden = false;
					for( Menu m1: Globals.HiddenMenuOptions )
					{
						if( m1.getClass().getName().equals( m.getClass().getName() ) )
							hidden = true;
					}
					if( ! hidden )
						menuStack.add(0, m);
				}
			}
			goBack(p);
		}
	}

	static void goBack(final MainActivity p)
	{
		if(menuStack.isEmpty())
		{
			Settings.Save(p);
			p.startDownloader();
		}
		else
		{
			Menu c = menuStack.remove(menuStack.size() - 1);
			c.run(p);
		}
	}

	static void goBackOuterMenu(final MainActivity p)
	{
		if(!menuStack.isEmpty())
			menuStack.remove(menuStack.size() - 1);
		goBack(p);
	}
	
	static class OkButton extends Menu
	{
		String title(final MainActivity p)
		{
			return p.getResources().getString(R.string.ok);
		}
		void run (final MainActivity p)
		{
			goBackOuterMenu(p);
		}
	}

	static class DummyMenu extends Menu
	{
		String title(final MainActivity p)
		{
			return p.getResources().getString(R.string.ok);
		}
		void run (final MainActivity p)
		{
			goBack(p);
		}
	}

	static class MainMenu extends Menu
	{
		String title(final MainActivity p)
		{
			return p.getResources().getString(R.string.device_config);
		}
		void run (final MainActivity p)
		{
			Menu options[] =
			{
				new SettingsMenuMisc.DownloadConfig(),
				new SettingsMenuMisc.OptionalDownloadConfig(false),
				new SettingsMenuKeyboard.KeyboardConfigMainMenu(),
				new SettingsMenuMouse.MouseConfigMainMenu(),
				new SettingsMenuMisc.GyroscopeCalibration(),
				new SettingsMenuMisc.AudioConfig(),
				new SettingsMenuKeyboard.RemapHwKeysConfig(),
				new SettingsMenuKeyboard.ScreenGesturesConfig(),
				new SettingsMenuMisc.VideoSettingsConfig(),
				new SettingsMenuMisc.ResetToDefaultsConfig(),
				new OkButton(),
			};
			showMenuOptionsList(p, options);
		}
	}
}
