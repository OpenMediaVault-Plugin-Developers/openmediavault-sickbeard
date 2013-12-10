<?php

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

require_once "openmediavault/object.inc";
require_once "openmediavault/config.inc";
require_once "openmediavault/error.inc";
require_once "openmediavault/util.inc";
require_once "openmediavault/rpcservice.inc";
require_once "openmediavault/notify.inc";

class OMVRpcServiceSickbeard extends OMVRpcServiceAbstract {

    public function getName() {
        return 'Sickbeard';
    }

    public function initialize() {
        $this->registerMethod('getSettings');
        $this->registerMethod('setSettings');
    }

    function getSettings($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Get configuration object
        $object = $xmlConfig->get("//services/sickbeard");
        if (is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, "//services/sickbeard");

        // Modify result data
        $object['enable']  = boolval($object['enable']);
        $object['showtab'] = boolval($object['showtab']);

        return $object;
    }

    function setSettings($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "enable"  : { "type" : "boolean" },
                "showtab" : { "type" : "boolean" }
            }
        }');

        // Prepare configuration data
        $object = array(
            "enable"  => array_boolval($params, 'enable'),
            "showtab" => array_boolval($params, 'showtab'),
        );

        // Set configuration object
        if (false === $xmlConfig->replace("//services/sickbeard", $object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED, "//services/sickbeard");

        // Notify configuration changes
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_MODIFY, "org.openmediavault.services.sickbeard", $object);
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceSickbeard());

?>