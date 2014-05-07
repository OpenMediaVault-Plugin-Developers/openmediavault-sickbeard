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
    extend : "OMV.workspace.form.Panel",
    uses   : [
        "OMV.data.Model",
        "OMV.data.Store"
    ],

    initComponent : function () {
        var me = this;

        me.on('load', function () {
            var checked = me.findField('enable').checked;
            var showtab = me.findField('showtab').checked;
            var parent = me.up('tabpanel');

            if (!parent)
                return;

            var managementPanel = parent.down('panel[title=' + _("Web Interface") + ']');

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
                xtype      : "combo",
                name       : "sb_repo",
                fieldLabel : "Repository",
                allowBlank : false,
                editable   : false,
                store   : [
                        [ 'a', _("Sickbeard - Main - midgetspy/Sick-Beard") ],
                        [ 'b', _("Sickbeard - Torrents - xbianonpi/Sick-Beard-TPB") ],
                        [ 'c', _("Sickbeard - Torrents - bricky/Sick-Beard") ],
                        [ 'd', _("Sickbeard - SickRage - echel0n/SickRage") ]
                    ],
                mode            : 'local',
                triggerAction   : 'all',
                selectOnFocus   : true,
                plugins       : [{
                    ptype : "fieldinfo",
                    text  : _("The repository you want to use. If changing from a current repository, setting will be wiped.")
                }]
            },{
                xtype      : "combo",
                name       : "sb_branch",
                fieldLabel : _("Branch"),
                queryMode  : "local",
                store      : Ext.create("Ext.data.ArrayStore", {
                    fields : [
                        "value",
                        "text"
                    ],
                    data   : [
                        [ 0, _("Master") ],
                        [ 1, _("Develop") ],
                        [ 2, _("Anime") ]
                    ]
                }),
                displayField  : "text",
                valueField    : "value",
                allowBlank    : false,
                editable      : false,
                triggerAction : "all",
                value         : 0,
                plugins       : [{
                    ptype : "fieldinfo",
                    text  : _("The branch you want to use.")
                }]
            },{
                xtype   : "button",
                name    : "opensickbeard",
                text    : _("Sickbeard Web Interface"),
                scope   : this,
                handler : function() {
                    var link = 'http://' + location.hostname + ':8081/';
                    window.open(link, '_blank');
                }
            },{
                border: false,
                html: "<br />"
            }]
        }];
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "settings",
    path      : "/service/sickbeard",
    text      : _("Settings"),
    position  : 10,
    className : "OMV.module.admin.service.sickbeard.Settings"
});
