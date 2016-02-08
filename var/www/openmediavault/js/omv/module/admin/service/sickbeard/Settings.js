/**
 * Copyright (C) 2013-2015 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/form/Panel.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/form/plugin/LinkedFields.js")

Ext.define("OMV.module.admin.service.sickbeard.Settings", {
    extend: "OMV.workspace.form.Panel",

    rpcService   : "Sickbeard",
    rpcGetMethod : "getInstance",
    rpcSetMethod : "setInstance",

    initComponent: function() {
        this.on("load", function() {
            var checked = this.findField("newinstance").value;
            var parent = this.up("tabpanel");

            if (!parent) {
                return;
            }

            var managementPanel = parent.down("panel[title=" + _("Instance 2") + "]");

            if (managementPanel) {
                checked ? managementPanel.enable()| managementPanel.tab.show() : managementPanel.disable()| managementPanel.tab.hide();
            }
        }, this);

        this.callParent(arguments);
    },

    getFormItems : function() {
        return [{
            xtype    : "fieldset",
            title    : "Instance settings",
            defaults : {
                labelSeparator : ""
            },
            items : [{
                xtype: "combo",
                name: "newinstance",
                fieldLabel: _("Instances"),
                queryMode: "local",
                store: Ext.create("Ext.data.ArrayStore", {
                    fields: [
                        "value",
                        "text"
                    ],
                    data: [
                        [0, _("Run single instance of SickBeard - Default.")],
                        [1, _("Run second instance of SickBeard.")]
                    ]
                }),
                displayField: "text",
                valueField: "value",
                allowBlank: false,
                editable: false,
                triggerAction: "all",
                value: 0,
                plugins: [{
                    ptype: "fieldinfo",
                    text: _("Will delete second instance files if set to single.")
                }]
            }]
        }];
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "settings",
    path      : "/service/sickbeard",
    text      : _("Settings"),
    position  : 70,
    className : "OMV.module.admin.service.sickbeard.Settings"
});


