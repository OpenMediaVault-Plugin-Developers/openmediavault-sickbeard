/**
 * Copyright (C) 2013 OpenMediaVault Plugin Developers
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
    extend   : "OMV.workspace.form.Panel",
    requires : [
        "OMV.data.Model",
        "OMV.data.Store"
    ],

    initComponent : function () {
        var me = this;

        me.on("load", function () {
            var checked = me.findField("enable").checked;
            var showtab = me.findField("showtab").checked;
            var parent = me.up("tabpanel");

            if (!parent)
                return;

            var managementPanel = parent.down("panel[title=" + _("Web Interface") + "]");

            if (managementPanel) {
                checked ? managementPanel.enable() : managementPanel.disable();
                showtab ? managementPanel.tab.show() : managementPanel.tab.hide();
            }
        });

        me.callParent(arguments);
    },

    rpcService   : "Sickbeard",
    rpcGetMethod : "getSettings",
    rpcSetMethod : "setSettings",

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

    getFormItems : function() {
        var me = this;

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
                allowBlank : false,
                editable   : false,
                queryMode  : "local",
                store      : Ext.create("OMV.data.Store", {
                    autoLoad : true,
                    model    : OMV.data.Model.createImplicit({
                        idProperty : "name",
                        fields     : [{
                            name : "uuid",
                            type : "string"
                        },{
                            name : "name",
                            type : "string"
                        },{
                            name : "fork",
                            type : "string"
                        },{
                            name : "branches",
                            type : "array"
                        }],
                        proxy : {
                            type    : "rpc",
                            rpcData : {
                                service : "Sickbeard",
                                method  : "enumerateRepos"
                            },
                            appendSortParams : false
                        }
                    })
                }),
                displayField  : "fork",
                valueField    : "fork",
                triggerAction : "all",
                selectOnFocus : true,
                plugins       : [{
                    ptype : "fieldinfo",
                    text  : _("The repository you want to use. If changing from a current repository, setting will be wiped.")
                }],
                listeners : {
                    select : function(combo, records) {
                        var record = records.pop();
                        me.updateBranchCombo(record.get("branches"));
                    },
                    change : function(combo, value) {
                        var record = combo.store.findRecord("fork", value);
                        me.updateBranchCombo(record.get("branches"));
                    }
                }
            },{
                xtype         : "combo",
                name          : "branch",
                fieldLabel    : _("Branch"),
                queryMode     : "local",
                store         : [],
                allowBlank    : false,
                editable      : false,
                triggerAction : "all",
                plugins       : [{
                    ptype : "fieldinfo",
                    text  : _("The branch you want to use. choose master if you don't know whats involed.")
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
            },{
                xtype   : "button",
                name    : "opensickbeard",
                text    : _("Sickbeard Web Interface"),
                scope   : this,
                handler : function() {
                    var me = this;
                    var link = "";
                    var proxy = me.getForm().findField("ppass").getValue();
                    
                    if (proxy == true) {
                        link = "http://" + location.hostname + "/sickbeard/home/";
                    } else {
                        var port = me.getForm().findField("port").getValue();
                        link = "http://" + location.hostname + ":" + port + "/home/";
                    }
                    
                    window.open(link, "_blank");
                },
                margin : "0 0 5 0"
            }]
                },{
                        xtype: "fieldset",
                        title: _("Backup User Settings"),
                        fieldDefaults: {
                                labelSeparator: ""
                        },
                        items : [{
                xtype         : "combo",
                name          : "mntentref",
                fieldLabel    : _("Volume"),
                emptyText     : _("Select a volume ..."),
                allowBlank    : false,
                allowNone     : false,
                editable      : false,
                triggerAction : "all",
                displayField  : "description",
                valueField    : "uuid",
                store         : Ext.create("OMV.data.Store", {
                    autoLoad : true,
                    model    : OMV.data.Model.createImplicit({
                        idProperty : "uuid",
                        fields     : [
                            { name : "uuid", type : "string" },
                            { name : "devicefile", type : "string" },
                            { name : "description", type : "string" }
                        ]
                    }),
                    proxy : {
                        type : "rpc",
                        rpcData : {
                            service : "ShareMgmt",
                            method  : "getCandidates"
                        },
                        appendSortParams : false
                    },
                    sorters : [{
                        direction : "ASC",
                        property  : "devicefile"
                    }]
                })
            },{
                xtype      : "textfield",
                name       : "path",
                fieldLabel : _("Path"),
                allowNone  : true,
                readOnly   : true
            },{
                xtype   : "button",
                name    : "backup",
                text    : _("Backup"),
                scope   : this,
                handler : Ext.Function.bind(me.onBackupButton, me, [ me ]),
                margin  : "5 0 0 0"
            },{
                border : false,
                html   : "<ul><li>" + _("Backup settings to a data drive.") + "</li></ul>"
            },{
                xtype   : "button",
                name    : "restore",
                text    : _("Restore"),
                scope   : this,
                handler : Ext.Function.bind(me.onRestoreButton, me, [ me ]),
                margin  : "5 0 0 0"
            },{
                border : false,
                html   : "<ul><li>" + _("Restore settings from a data drive.") + "</li></ul>"
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
                allowBlank : false,
                editable   : false,
                queryMode  : "local",
                store      : Ext.create("OMV.data.Store", {
                    autoLoad : true,
                    model    : OMV.data.Model.createImplicit({
                        idProperty : "name",
                        fields     : [{
                            name : "uuid",
                            type : "string"
                        },{
                            name : "name",
                            type : "string"
                        },{
                            name : "fork",
                            type : "string"
                        },{
                            name : "branches",
                            type : "array"
                        }],
                        proxy : {
                            type    : "rpc",
                            rpcData : {
                                service : "Sickbeard",
                                method  : "enumerateRepos"
                            },
                            appendSortParams : false
                        }
                    })
                }),
                displayField  : "fork",
                valueField    : "fork",
                triggerAction : "all",
                selectOnFocus : true,
                plugins       : [{
                    ptype : "fieldinfo",
                    text  : _("The repository you want to use. If changing from a current repository, setting will be wiped.")
                }],
                listeners : {
                    select : function(combo, records) {
                        var record = records.pop();
                        me.updateBranchCombo2(record.get("branches"));
                    },
                    change : function(combo, value) {
                        var record = combo.store.findRecord("fork", value);
                        me.updateBranchCombo2(record.get("branches"));
                    }
                }
            },{
                xtype         : "combo",
                name          : "branch2",
                fieldLabel    : _("Branch"),
                queryMode     : "local",
                store         : [],
                allowBlank    : false,
                editable      : false,
                triggerAction : "all",
                plugins       : [{
                    ptype : "fieldinfo",
                    text  : _("The branch you want to use. choose master if you don't know whats involed.")
                }]
            }]
        }];
    },

    onBackupButton: function() {
        var me = this;
        me.doSubmit();
        Ext.create("OMV.window.Execute", {
            title      : _("Backup"),
            rpcService : "Sickbeard",
            rpcMethod  : "doBackup",
            listeners  : {
                scope     : me,
                exception : function(wnd, error) {
                    OMV.MessageBox.error(null, error);
                }
            }
        }).show();
    },

    onRestoreButton: function() {
        var me = this;
        me.doSubmit();
        Ext.create("OMV.window.Execute", {
            title      : _("Restore"),
            rpcService : "Sickbeard",
            rpcMethod  : "doRestore",
            listeners  : {
                scope     : me,
                exception : function(wnd, error) {
                    OMV.MessageBox.error(null, error);
                }
            }
        }).show();
    },

    updateBranchCombo : function(values) {
        var me = this;
        var branchCombo = me.findField("branch");

        branchCombo.store.removeAll();

        for (var i = 0; i < values.length; i++) {
            // TODO: Look over use of field1
            branchCombo.store.add({ field1: values[i] });
        }
    },

    updateBranchCombo2 : function(values) {
        var me = this;
        var branchCombo2 = me.findField("branch2");

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
