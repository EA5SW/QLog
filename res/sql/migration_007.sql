UPDATE contacts SET address_intl = address WHERE address_intl IS NULL AND address IS NOT NULL;
UPDATE contacts SET comment_intl = comment WHERE comment_intl IS NULL AND comment IS NOT NULL;
UPDATE contacts SET country_intl = country WHERE country_intl IS NULL AND country IS NOT NULL;
UPDATE contacts SET my_antenna_intl = my_antenna WHERE my_antenna_intl IS NULL AND my_antenna IS NOT NULL;
UPDATE contacts SET my_city_intl = my_city WHERE my_city_intl IS NULL AND my_city IS NOT NULL;
UPDATE contacts SET my_country_intl = my_country WHERE my_country_intl IS NULL AND my_country IS NOT NULL;
UPDATE contacts SET my_name_intl = my_name WHERE my_name_intl IS NULL AND my_name IS NOT NULL;
UPDATE contacts SET my_postal_code_intl = my_postal_code WHERE my_postal_code_intl IS NULL AND my_postal_code IS NOT NULL;
UPDATE contacts SET my_rig_intl = my_rig WHERE my_rig_intl IS NULL AND my_rig IS NOT NULL;
UPDATE contacts SET my_sig_intl = my_sig WHERE my_sig_intl IS NULL AND my_sig IS NOT NULL;
UPDATE contacts SET my_sig_info_intl = my_sig_info WHERE my_sig_info_intl IS NULL AND my_sig_info IS NOT NULL;
UPDATE contacts SET my_street_intl = my_street WHERE my_street_intl IS NULL AND my_street IS NOT NULL;
UPDATE contacts SET name_intl = name WHERE name_intl IS NULL AND name IS NOT NULL;
UPDATE contacts SET notes_intl = notes WHERE notes_intl IS NULL AND notes IS NOT NULL;
UPDATE contacts SET qth_intl = qth WHERE qth_intl IS NULL AND qth IS NOT NULL;
UPDATE contacts SET rig_intl = rig WHERE rig_intl IS NULL AND rig IS NOT NULL;
UPDATE contacts SET sig_intl = sig WHERE sig_intl IS NULL AND sig IS NOT NULL;
UPDATE contacts SET sig_info_intl = sig_info WHERE sig_info_intl IS NULL AND sig_info IS NOT NULL;
UPDATE contacts SET clublog_qso_upload_status='N' WHERE clublog_qso_upload_status IS NULL;
UPDATE contacts SET qrzcom_qso_upload_status='N' WHERE qrzcom_qso_upload_status IS NULL;
UPDATE contacts SET hrdlog_qso_upload_status='N' WHERE hrdlog_qso_upload_status IS NULL;
UPDATE contacts SET eqsl_qsl_rcvd='N' WHERE eqsl_qsl_rcvd IS NULL;
UPDATE contacts SET eqsl_qsl_sent='N' WHERE eqsl_qsl_sent IS NULL;


ALTER TABLE rig_profiles ADD default_pwr REAL DEFAULT 100.0;
