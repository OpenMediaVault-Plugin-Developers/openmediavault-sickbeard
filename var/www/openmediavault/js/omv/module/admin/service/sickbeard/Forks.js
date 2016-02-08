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
// require("js/omv/workspace/grid/Panel.js")
// require("js/omv/Rpc.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/module/admin/service/sickbeard/Fork.js")

Ext.define("OMV.module.admin.service.sickbeard.Forks", {
    extend: "OMV.workspace.grid.Panel",
    requires: [
        "OMV.Rpc",
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc",
        "OMV.module.admin.service.sickbeard.Fork"
    ],

    hidePagingToolbar: false,

    columns: [{
        text: _("Name"),
        sortable: true,
        dataIndex: "name",
        stateId: "name"
    }, {
        text: _("Fork"),
        sortable: true,
        dataIndex: "fork",
        stateId: "fork"
    }],

    store: Ext.create("OMV.data.Store", {
        autoLoad: true,
        model: OMV.data.Model.createImplicit({
            idProperty: "uuid",
            fields: [{
                name: "uuid",
                type: "string"
            }, {
                name: "name",
                type: "string"
            }, {
                name: "fork",
                type: "string"
            }]
        }),
        proxy: {
            type: "rpc",
            rpcData: {
                service: "Sickbeard",
                method: "getForks"
            }
        }
    }),

    onAddButton: function() {
        Ext.create("OMV.module.admin.service.sickbeard.Fork", {
            title: _("Add a git fork"),
            uuid: OMV.UUID_UNDEFINED,
            listeners: {
                scope: this,
                submit: function() {
                    document.location.reload();
                }
            }
        }).show();
    },

    onEditButton: function() {
        var record = this.getSelected();

        Ext.create("OMV.module.admin.service.sickbeard.Fork", {
            title: _("Edit fork"),
            uuid: record.get("uuid"),
            listeners: {
                scope: this,
                submit: function() {
                    document.location.reload();
                }
            }
        }).show();
    },

    doDeletion: function(record) {
        OMV.Rpc.request({
            scope: this,
            callback: this.onDeletion,
            rpcData: {
                service: "Sickbeard",
                method: "deleteFork",
                params: {
                    uuid: record.get("uuid")
                }
            }
        });
    }
});

OMV.WorkspaceManager.registerPanel({
    id: "scheduledjobs",
    path: "/service/sickbeard",
    text: _("Forks"),
    position: 60,
    className: "OMV.module.admin.service.sickbeard.Forks"
});


