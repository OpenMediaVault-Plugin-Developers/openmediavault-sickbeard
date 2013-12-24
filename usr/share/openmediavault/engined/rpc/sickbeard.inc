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
    function getSettings($params, $context) {
        global $xmlConfig;
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR ));
        $object = $xmlConfig->get("//services/sickbeard");

        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, "//services/sickbeard");

        $object['enable'] = boolval($object['enable']);
        $object['showtab'] = boolval($object['showtab']);
        $object['sb_branch'] = intval($object['sb_branch']);
        return $object;
   }

    function setSettings($params, $context) {
        global $xmlConfig;
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));
        $this->validateMethodParams($params, '{
            "type" : "object",
            "properties" : {
                "enable"        : { "type" : "boolean"},
                "showtab"       : { "type" : "boolean"},
                "sb_repo"       : { "type" : "string"},
                "sb_branch"     : { "type" : "integer","enum":[0,1]}
            }
        }');

        $xpath = "//services/sickbeard";
        $oldObject = $xmlConfig->get($xpath);

        if (is_null($oldObject))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        if($params['sb_repo'] != 'a' && $params['sb_branch'] == 1)
            throw new OMVException(OMVErrorMsg::E_MISC_FAILURE, "You CAN NOT use this branch with this repository.");

        if($params['sb_repo'] != $oldObject['sb_repo']) {
            $params['sb_branch'] = 0;
            $cmd = "omv-mkconf sickbeard swap_" . $params['sb_repo'] . " 2>&1";
        } else {
            if($params['sb_branch'] != $oldObject['sb_branch']) {
                if($params['sb_branch'] == 0) {
                    $cmd = "omv-mkconf sickbeard master 2>&1";
                }
                if($params['sb_branch'] == 1) {
                    $cmd = "omv-mkconf sickbeard develop 2>&1";
                }
            }
        }

        $this->exec($cmd);
        $object = array(
            "enable"            => array_boolval($params, 'enable'),
            "showtab"           => array_boolval($params, 'showtab'),
            "sb_branch"         => $params['sb_branch'],
            "sb_repo"           => $params['sb_repo']
        );

        if(false === $xmlConfig->replace("//services/sickbeard", $object)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED, "//services/sickbeard");
        }

        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_MODIFY, "org.openmediavault.services.sickbeard", $object);
    }

}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceSickbeard());

?>
