<?php

/**
 * Copyright (C)      2013 OpenMediaVault Plugin Developers
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

require_once "openmediavault/module.inc";
require_once "openmediavault/config.inc";
require_once "openmediavault/error.inc";
require_once "openmediavault/util.inc";
require_once "openmediavault/initscript.inc";

class OMVModuleSickbeard extends OMVModuleServiceAbstract
    implements OMVINotifyListener, OMVIModuleServiceStatus {

    public function getName() {
        return "Sickbeard";
    }

    public function getStatus() {
        global $xmlConfig;

        $object = $xmlConfig->get("//services/sickbeard");

        if (is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, "//services/sickbeard");

        $cmd = "/etc/init.d/sickbeard status";
        $this->exec($cmd, $output);

        return array(
            "name"    => $this->getName(),
            "title"   => gettext("Sickbeard"),
            "enabled" => boolval($object['enable']),
            "running" => (strpos($output[0], 'is running') > 0)
        );
    }

    public function startService() {
        global $xmlConfig;

        $object = $xmlConfig->get("//services/sickbeard");

        if (is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, "//services/sickbeard");

        if (boolval($object['enable']) === false)
            return;

        $cmd = "sudo /etc/init.d/sickbeard start 2>&1";
        if(0 !== $this->exec($cmd, $output))
            throw new OMVException(OMVErrorMsg::E_EXEC_FAILED, $cmd, implode("\n", $output));
    }

    public function stopService() {
        global $xmlConfig;

        $object = $xmlConfig->get("//services/sickbeard");

        if (is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, "//services/sickbeard");

        if (boolval($object['enable']) === true)
            return;

        $cmd = "sudo /etc/init.d/sickbeard stop 2>&1";
        if(0 !== $this->exec($cmd, $output))
            throw new OMVException(OMVErrorMsg::E_EXEC_FAILED, $cmd, implode("\n", $output));
    }

    public function bindListeners(OMVNotifyDispatcher $dispatcher) {
        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            "org.openmediavault.services.sickbeard",
            array($this, "setDirty"));
    }
}

// Register module
$moduleMgr = &OMVModuleMgr::getInstance();
$moduleMgr->registerModule(new OMVModuleSickbeard());
