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
// require("js/omv/module/admin/service/sickbeard/Backup.js")

Ext.define("OMV.module.admin.service.sickbeard.Settings", {
    extend: "OMV.workspace.form.Panel",
    requires: [
        "OMV.data.Model",
        "OMV.data.Store",
        "OMV.module.admin.service.sickbeard.Backup"
    ],

    rpcService   : "Sickbeard",
    rpcGetMethod : "getSettings",
    rpcSetMethod : "setSettings",

    initComponent: function() {
        this.on("load", function() {
            var checked = this.findField("enable").checked;
            var showtab = this.findField("showtab").checked;
            var parent = this.up("tabpanel");

            if (!parent) {
                return;
            }

            var managementPanel = parent.down("panel[title=" + _("Web Interface") + "]");

            if (managementPanel) {
                checked ? managementPanel.enable() : managementPanel.disable();
                showtab ? managementPanel.tab.show() : managementPanel.tab.hide();
            }
        }, this);

        this.callParent(arguments);
    },

    plugins      : [{ 
        ptype        : "linkedfields", 
        correlations : [{ 
            name       : [ 
                "port", 
            ], 
            properties : "!show" 
        },{ 
            name       : [ 
                "newinstenable", 
                "repo2", 
                "branch2", 
            ], 
            conditions : [ 
                { name  : "newinstance", value : false } 
            ], 
            properties : "!show" 
        }] 
    }],

    getButtonItems: function() {
        var items = this.callParent(arguments);

        items.push({
            id: this.getId() + "-show",
            xtype: "button",
            text: _("SB Web Client"),
            icon: "images/sickbeard.png",
            iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
            scope: this,
            handler: function() {
                var port = this.getForm().findField("port").getValue();
                var link = "http://" + location.hostname + ":" + port + "/";

                window.open(link, "_blank");
            }
        }, {
            id: this.getId() + "-backup",
            xtype: "button",
            text: _("Backup/restore"),
            icon: "images/wrench.png",
            iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
            scope: this,
            handler: function() {
                Ext.create("OMV.module.admin.service.sickbeard.Backup").show();
            }
        });

        return items;
    },

    getFormItems : function() {
        return [{
            xtype    : "fieldset",
            title    : "General settings",
            defaults : {
                labelSeparator : ""
            },
            items : [{
                xtype      : "checkbox",
                name       : "enable",
                fieldLabel : _("Enable"),
                checked    : false
            },{
                xtype      : "checkbox",
                name       : "showtab",
                fieldLabel : _("Show Tab"),
                boxLabel   : _("Show tab containing Sickbeard web interface frame."),
                checked    : false
            },{
                xtype      : "checkbox",
                name       : "ssl",
                fieldLabel : _("SSL"),
                boxLabel   : _("Auto enable SSL. An OpenMediaVault certificate must have been generated."),
                checked    : false
            },{
                xtype      : "checkbox",
                name       : "ppass",
                fieldLabel : _("Proxy Pass"),
                boxLabel   : _("Enable this to access via OMV_IP/sickbeard"),
                checked    : false
            },{
                xtype      : "combo",
                name       : "repo",
                fieldLabel : _("Repository"),
                store: Ext.create("OMV.data.Store", {
                    autoLoad: true,
                    model: OMV.data.Model.createImplicit({
                        idProperty: "name",
                        fields: [{
                            name: "uuid",
                            type: "string"
                        }, {
                            name: "name",
                            type: "string"
                        }, {
                            name: "fork",
                            type: "string"
                        }, {
                            name: "branches",
                            type: "array"
                        }],
                        proxy: {
                            type    : "rpc",
                            rpcData : {
                                service : "Sickbeard",
                                method  : "enumerateRepos"
                            },
                            appendSortParams : false
                        }
                    })
                }),
                allowBlank: false,
                displayField: "fork",
                editable: false,
                listeners: {
                    scope: this,
                    change: function(combo, value) {
                        var record = combo.store.findRecord("fork", value);

                        if (record != null) {
                            this.updateBranchCombo(record.get("branches"));
                        }
                    },
                    select: function(combo, records) {
                        var record = records.pop();

                        if (record != null) {
                            this.updateBranchCombo(record.get("branches"));
                        }
                    }
                },
                queryMode: "local",
                selectOnFocus: true,
                triggerAction: "all",
                valueField: "fork",
                plugins: [{
                    ptype: "fieldinfo",
                    text: _("The repository you want to use. If changing from a current repository, setting will be wiped.")
                }]
            }, {
                xtype: "combo",
                name: "branch",
                fieldLabel: _("Branch"),
                allowBlank: false,
                editable: false,
                queryMode: "local",
                store: [],
                triggerAction: "all",
                plugins: [{
                    ptype: "fieldinfo",
                    text: _("The branch you want to use. choose master if you don't know what's involved.")
                }]
            },{
                xtype: "numberfield",
                name: "port",
                fieldLabel: _("Port"),
                vtype: "port",
                minValue: 1,
                maxValue: 65535,
                allowDecimals: false,
                allowBlank: false,
                value: 8081
            }]
                },{
                        xtype    : "fieldset",
            title    : "Second version",
            defaults : {
                labelSeparator : ""
            },
            items : [{
                xtype      : "checkbox",
                name       : "newinstance",
                fieldLabel : _("Enable"),
                boxLabel   : _("Will create second configuration. Unticking will remove everything."),
                checked    : false
            },{
                xtype      : "checkbox",
                name       : "newinstenable",
                fieldLabel : _("Run"),
                boxLabel   : _("Will run the second instance of SickBeard. Use to start/stop the second service."),
                checked    : false
            },{
                xtype      : "combo",
                name       : "repo2",
                fieldLabel : _("Repository"),
                store: Ext.create("OMV.data.Store", {
                    autoLoad: true,
                    model: OMV.data.Model.createImplicit({
                        idProperty: "name",
                        fields: [{
                            name: "uuid",
                            type: "string"
                        }, {
                            name: "name",
                            type: "string"
                        }, {
                            name: "fork",
                            type: "string"
                        }, {
                            name: "branches",
                            type: "array"
                        }],
                        proxy: {
                            type    : "rpc",
                            rpcData : {
                                service : "Sickbeard",
                                method  : "enumerateRepos"
                            },
                            appendSortParams : false
                        }
                    })
                }),
                allowBlank: true,
                displayField: "fork",
                editable: false,
                listeners: {
                    scope: this,
                    change: function(combo, value) {
                        var record = combo.store.findRecord("fork", value);

                        if (record != null) {
                            this.updateBranchCombo2(record.get("branches"));
                        }
                    },
                    select: function(combo, records) {
                        var record = records.pop();

                        if (record != null) {
                            this.updateBranchCombo2(record.get("branches"));
                        }
                    }
                },
                queryMode: "local",
                selectOnFocus: true,
                triggerAction: "all",
                valueField: "fork",
                plugins: [{
                    ptype: "fieldinfo",
                    text: _("The repository you want to use. If changing from a current repository, setting will be wiped.")
                }]
            }, {
                xtype: "combo",
                name: "branch2",
                fieldLabel: _("Branch"),
                allowBlank: false,
                editable: false,
                queryMode: "local",
                store: [],
                triggerAction: "all",
                plugins: [{
                    ptype: "fieldinfo",
                    text: _("The branch you want to use. choose master if you don't know what's involved.")
                }]
            }]
        }];
    },

    updateBranchCombo : function(values) {
        var branchCombo = this.findField("branch");

        branchCombo.store.removeAll();

        for (var i = 0; i < values.length; i++) {
            // TODO: Look over use of field1
            branchCombo.store.add({ field1: values[i] });
        }
    },

    updateBranchCombo2 : function(values) {
        var branchCombo2 = this.findField("branch2");

        branchCombo2.store.removeAll();

        for (var i = 0; i < values.length; i++) {
            // TODO: Look over use of field1
            branchCombo2.store.add({ field1: values[i] });
        }
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "settings",
    path      : "/service/sickbeard",
    text      : _("Settings"),
    position  : 10,
    className : "OMV.module.admin.service.sickbeard.Settings"
});

