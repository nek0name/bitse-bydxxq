package com.chargingplatform.user;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.location.Address;
import android.location.Geocoder;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsAnimation;
import android.view.Window;
import android.view.WindowManager;
import android.webkit.JavascriptInterface;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Toast;
import com.lbt05.EvilTransform.GCJPointer;
import com.lbt05.EvilTransform.WGSPointer;
import java.util.List;
import java.util.Locale;

public final class NativeMobile {
  private static final Handler handler = new Handler(Looper.getMainLooper());
  private static LocationListener listener;
  private static LocationManager manager;
  private static Runnable timeout;
  private static int requestId;
  private static native void keyboardInsets(double inset);
  private static native void locationResult(double latitude, double longitude, String name);
  private static native void locationError(String message);

  public static void configureKeyboard(Context context) {
    Activity activity = (Activity) context;
    // Version-qualified manifest resources select adjustNothing from API 30,
    // preserving adjustResize on earlier Android versions. Qt reads the same
    // ActivityInfo value whenever it shows the IME.
    if (android.os.Build.VERSION.SDK_INT < 30) return;
    View decor = activity.getWindow().getDecorView();
    float density = activity.getResources().getDisplayMetrics().density;
    java.util.function.Consumer<WindowInsets> report = insets -> {
      int ime = insets.isVisible(WindowInsets.Type.ime())
        ? insets.getInsets(WindowInsets.Type.ime()).bottom : 0;
      int navigation = insets.getInsets(WindowInsets.Type.navigationBars()).bottom;
      keyboardInsets(Math.max(0, ime - navigation) / density);
    };
    // Qt 6.7.3 observes its QtRootLayout with OnPreDrawListener; it does not
    // install a decor insets listener/callback. Preserve DecorView processing.
    boolean[] imeAnimating = {false};
    decor.setOnApplyWindowInsetsListener((view, insets) -> {
      if (!imeAnimating[0]) report.accept(insets);
      return view.onApplyWindowInsets(insets);
    });
    decor.setWindowInsetsAnimationCallback(new WindowInsetsAnimation.Callback(
        WindowInsetsAnimation.Callback.DISPATCH_MODE_CONTINUE_ON_SUBTREE) {
      @Override public void onPrepare(WindowInsetsAnimation animation) {
        if ((animation.getTypeMask() & WindowInsets.Type.ime()) != 0) imeAnimating[0] = true;
      }
      @Override public void onEnd(WindowInsetsAnimation animation) {
        if ((animation.getTypeMask() & WindowInsets.Type.ime()) == 0) return;
        imeAnimating[0] = false;
        WindowInsets current = decor.getRootWindowInsets();
        if (current != null) report.accept(current);
      }
      @Override public WindowInsets onProgress(WindowInsets insets,
          List<WindowInsetsAnimation> runningAnimations) {
        report.accept(insets);
        return insets;
      }
    });
    WindowInsets current = decor.getRootWindowInsets();
    if (current != null) report.accept(current);
    else keyboardInsets(0);
    decor.requestApplyInsets();
  }

  public static void updateBars(Context context, int color, boolean dark) {
    Window window = ((Activity) context).getWindow();
    window.setStatusBarColor(color);
    window.setNavigationBarColor(color);
    window.getDecorView().setBackgroundColor(color);
    int flags = window.getDecorView().getSystemUiVisibility();
    flags = dark ? flags & ~8192 : flags | 8192;
    if (android.os.Build.VERSION.SDK_INT >= 26) flags = dark ? flags & ~16 : flags | 16;
    window.getDecorView().setSystemUiVisibility(flags);
  }

  public static void toast(Context context, String message) {
    Toast.makeText(context, message, Toast.LENGTH_SHORT).show();
  }

  private static void cancelLocation() {
    if (manager != null && listener != null) manager.removeUpdates(listener);
    if (timeout != null) handler.removeCallbacks(timeout);
    listener = null;
    timeout = null;
  }

  private static String cleanAddressPart(String value) {
    return value == null ? "" : value.trim();
  }

  private static boolean isCountryLabel(String value, String country) {
    return !value.isEmpty() && (value.equalsIgnoreCase(country)
      || value.equals("中国") || value.equalsIgnoreCase("China") || value.equalsIgnoreCase("CN"));
  }

  private static String detailedAddress(Address address) {
    java.util.LinkedHashSet<String> parts = new java.util.LinkedHashSet<>();
    String country = cleanAddressPart(address.getCountryName());
    String district = cleanAddressPart(address.getSubLocality());
    if (district.isEmpty()) district = cleanAddressPart(address.getSubAdminArea());
    String city = cleanAddressPart(address.getLocality());
    if (city.isEmpty()) city = cleanAddressPart(address.getAdminArea());
    String road = cleanAddressPart(address.getThoroughfare());
    String number = cleanAddressPart(address.getSubThoroughfare());
    for (String part : new String[]{city, district, road}) {
      if (!part.isEmpty() && !isCountryLabel(part, country)) parts.add(part);
    }
    if (!number.isEmpty() && !road.isEmpty() && !isCountryLabel(road, country)
        && !road.contains(number)) parts.add(number);
    String feature = cleanAddressPart(address.getFeatureName());
    boolean duplicateFeature = false;
    if (!feature.isEmpty()) for (String part : parts) {
      if (part.contains(feature)) duplicateFeature = true;
    }
    if (!feature.isEmpty() && !isCountryLabel(feature, country)
        && !feature.equals(cleanAddressPart(address.getPostalCode())) && !feature.equals(number)
        && !duplicateFeature) parts.add(feature);
    // Strip separators before checking the country, including providers that
    // return only a country padded by spaces or punctuation.
    String line = cleanAddressPart(address.getAddressLine(0))
      .replaceAll("^[,，\\s]+|[,，\\s]+$", "");
    if (!country.isEmpty() && line.startsWith(country)) {
      line = line.substring(country.length()).trim()
        .replaceAll("^[,，\\s]+|[,，\\s]+$", "");
    }
    if (isCountryLabel(line, country)) line = "";
    // A provider may expose only the city in structured fields while keeping
    // the district and street in addressLine. Keep that more precise address.
    boolean onlyCity = !city.isEmpty() && parts.size() == 1 && parts.contains(city);
    if (parts.isEmpty() || (onlyCity && line.length() > city.length())) return line;
    return android.text.TextUtils.join(" · ", parts);
  }

  private static String coordinateLabel(double latitude, double longitude) {
    return String.format(Locale.CHINA, "%.4f°%s · %.4f°%s", Math.abs(latitude),
      latitude < 0 ? "S" : "N", Math.abs(longitude), longitude < 0 ? "W" : "E");
  }

  private static void deliver(Location location, Context context, int id) {
    cancelLocation();
    GCJPointer point = new WGSPointer(location.getLatitude(), location.getLongitude()).toGCJPointer();
    locationResult(point.getLatitude(), point.getLongitude(), coordinateLabel(location.getLatitude(), location.getLongitude()));
    // Coordinates remain usable even when the system geocoder has no address.
    new Thread(() -> {
      try {
        List<Address> results = new Geocoder(context, Locale.CHINA)
          .getFromLocation(location.getLatitude(), location.getLongitude(), 5);
        if (results == null || results.isEmpty()) return;
        String best = "";
        int bestScore = -1;
        for (Address address : results) {
          String candidate = detailedAddress(address);
          if (candidate.isEmpty()) continue;
          int score = (address.getThoroughfare() != null ? 8 : 0)
            + (address.getSubLocality() != null ? 4 : 0)
            + (address.getLocality() != null ? 2 : 0);
          if (score > bestScore) { best = candidate; bestScore = score; }
        }
        final String name = best;
        if (!name.isEmpty()) handler.post(() -> {
          if (requestId == id) locationResult(point.getLatitude(), point.getLongitude(), name);
        });
      } catch (Exception ignored) { }
    }, "location-address").start();
  }

  public static void locate(Context context) {
    cancelLocation();
    int id = ++requestId;
    manager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
    boolean fine = context.checkSelfPermission(android.Manifest.permission.ACCESS_FINE_LOCATION)
      == android.content.pm.PackageManager.PERMISSION_GRANTED;
    boolean gps = fine && manager.isProviderEnabled(LocationManager.GPS_PROVIDER);
    boolean network = manager.isProviderEnabled(LocationManager.NETWORK_PROVIDER);
    if (!gps && !network) {
      locationError("系统定位已关闭，请在手机设置中开启定位");
      return;
    }
    listener = new LocationListener() {
      @Override public void onLocationChanged(Location location) {
        if (id == requestId && listener != null) deliver(location, context, id);
      }
      @Override public void onProviderEnabled(String provider) { }
      @Override public void onProviderDisabled(String provider) { }
      @Override public void onStatusChanged(String provider, int status, Bundle extras) { }
    };
    try {
      Location recent = null;
      for (String provider : new String[]{LocationManager.GPS_PROVIDER, LocationManager.NETWORK_PROVIDER}) {
        if (provider.equals(LocationManager.GPS_PROVIDER) && !fine) continue;
        if (!manager.isProviderEnabled(provider)) continue;
        Location last = manager.getLastKnownLocation(provider);
        if (last != null && SystemClock.elapsedRealtimeNanos() - last.getElapsedRealtimeNanos() < 120000000000L
            && last.hasAccuracy() && last.getAccuracy() <= 300
            && (recent == null || last.getElapsedRealtimeNanos() > recent.getElapsedRealtimeNanos())) recent = last;
        manager.requestLocationUpdates(provider, 1000, 0, listener, Looper.getMainLooper());
      }
      if (recent != null) { deliver(recent, context, id); return; }
      timeout = () -> {
        if (requestId != id) return;
        cancelLocation();
        locationError("暂时无法获取当前位置，可重试或在地图上手动选择");
      };
      handler.postDelayed(timeout, 25000);
    } catch (SecurityException error) {
      cancelLocation();
      locationError("未获得定位权限，请在手机设置中允许定位");
    } catch (Exception error) {
      cancelLocation();
      locationError("系统定位暂不可用，请在地图上选择位置");
    }
  }

  public static void pickLocation(Context context, double latitude, double longitude, boolean known, String theme) {
    ++requestId;
    cancelLocation();
    Activity activity = (Activity) context;
    org.json.JSONObject palette;
    try { palette = new org.json.JSONObject(theme); }
    catch (org.json.JSONException error) { palette = new org.json.JSONObject(); }
    boolean dark = palette.optBoolean("dark", false);
    int background = android.graphics.Color.parseColor(palette.optString("paper", "#f7f7f8"));
    Dialog dialog = new Dialog(activity, dark
      ? android.R.style.Theme_Material_NoActionBar : android.R.style.Theme_Material_Light_NoActionBar);
    dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
    WebView web = new WebView(activity);
    web.setBackgroundColor(background);
    web.getSettings().setJavaScriptEnabled(true);
    web.getSettings().setDomStorageEnabled(true);
    web.getSettings().setAllowFileAccess(false);
    web.getSettings().setAllowContentAccess(false);
    web.getSettings().setUserAgentString(web.getSettings().getUserAgentString()
      + " ChargingPlatform/1.0.3 (+https://github.com/windlandneko/bitse-bydxxq)");
    web.setWebViewClient(new WebViewClient() {
      @Override public boolean shouldOverrideUrlLoading(WebView view, String url) {
        // Only bundled map code may access the narrow selection bridge.
        return true;
      }
    });
    web.addJavascriptInterface(new Object() {
      @JavascriptInterface public void confirm(double lat, double lon) {
        if (Double.isNaN(lat) || Double.isInfinite(lat) || Double.isNaN(lon) || Double.isInfinite(lon)
            || Math.abs(lat) > 85 || Math.abs(lon) > 180) return;
        activity.runOnUiThread(() -> {
          GCJPointer point = new WGSPointer(lat, lon).toGCJPointer();
          locationResult(point.getLatitude(), point.getLongitude(),
            String.format(Locale.CHINA, "地图选点 · %.4f, %.4f", lat, lon));
          dialog.dismiss();
        });
      }
      @JavascriptInterface public void cancel() { activity.runOnUiThread(dialog::dismiss); }
    }, "LocationPicker");
    dialog.setContentView(web);
    dialog.setOnDismissListener(ignored -> { web.removeJavascriptInterface("LocationPicker"); web.destroy(); });
    dialog.show();
    Window mapWindow = dialog.getWindow();
    mapWindow.setLayout(WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.MATCH_PARENT);
    mapWindow.setStatusBarColor(background);
    mapWindow.setNavigationBarColor(background);
    mapWindow.getDecorView().setBackgroundColor(background);
    int flags = mapWindow.getDecorView().getSystemUiVisibility();
    flags = dark ? flags & ~8192 : flags | 8192;
    if (android.os.Build.VERSION.SDK_INT >= 26) flags = dark ? flags & ~16 : flags | 16;
    mapWindow.getDecorView().setSystemUiVisibility(flags);
    double lat = 35, lon = 105;
    if (known) {
      WGSPointer point = new GCJPointer(latitude, longitude).toWGSPointer();
      lat = point.getLatitude(); lon = point.getLongitude();
    }
    web.loadUrl("file:///android_asset/map/index.html?lat=" + lat + "&lon=" + lon + "&known=" + known
      + "&theme=" + android.net.Uri.encode(theme));
  }
}
