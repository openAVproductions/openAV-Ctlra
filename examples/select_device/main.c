// inspired by ComboBox sample code here
// https://developer.gnome.org/gnome-devel-demos/stable/combobox.c.html.en
// https://developer.gnome.org/gtk-tutorial/stable/x1063.html
#include <gtk/gtk.h>

#include <ctlra/ctlra.h>

struct app_t
{
	GtkWidget *combo_vendor;
	GtkWidget *combo_device;
	GtkWidget *vbox;
};

static void
on_changed (GtkComboBox *widget, gpointer user_data)
{
	struct app_t *app = user_data;

	GtkComboBox *combo_vendor = widget;
	if (gtk_combo_box_get_active (combo_vendor) == 0)
		return;

	gtk_widget_set_sensitive (GTK_WIDGET(combo_vendor), 0);
	gchar *vendor = gtk_combo_box_text_get_active_text
				(GTK_COMBO_BOX_TEXT(combo_vendor));

	const char *devices[32];
	int devs = ctlra_get_devices_by_vendor(vendor, devices, 32);

	app->combo_device = gtk_combo_box_text_new ();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(app->combo_device),
					"Choose a device");
	for(int i = 0; i < devs; i++) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_device),
						devices[i]);
	}

	gtk_container_add (GTK_CONTAINER (app->vbox), app->combo_device);
	gtk_widget_show_all(app->vbox);

	g_free (vendor);
}

static void
activate (GtkApplication *gtk_app,
          gpointer        user_data)
{
	gint i;
	GtkWidget *window;
	struct app_t *app = user_data;

	window = gtk_application_window_new (gtk_app);
	gtk_window_set_title (GTK_WINDOW (window), "Ctlra Device Chooser");
	gtk_window_set_default_size (GTK_WINDOW (window), 200, -1);
	gtk_container_set_border_width (GTK_CONTAINER (window), 10);

	app->combo_vendor = gtk_combo_box_text_new ();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(app->combo_vendor),
					"Choose a Vendor");

	const char *vendors[32];
	int ret = ctlra_get_vendors(vendors, 32);
	for (i = 0; i < ret; i++) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_vendor),
						vendors[i]);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (app->combo_vendor), 0);

	app->vbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	g_signal_connect (app->combo_vendor, "changed",
			  G_CALLBACK(on_changed), app);

	gtk_container_add (GTK_CONTAINER (window), app->vbox);
	gtk_container_add (GTK_CONTAINER (app->vbox), app->combo_vendor);
	gtk_widget_show_all (window);
}


int
main (int argc, char **argv)
{
	GtkApplication *gtk_app;
	int status;

	gtk_app = gtk_application_new ("com.openavproductions.ctlra.example",
				   G_APPLICATION_FLAGS_NONE);

	struct app_t app = {0};
	g_signal_connect (gtk_app, "activate", G_CALLBACK (activate), &app);
	status = g_application_run (G_APPLICATION (gtk_app), argc, argv);
	g_object_unref (gtk_app);

	return status;
}
